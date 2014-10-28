#include <gtest/gtest.h>

#include <gfal_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/exceptions/gerror_to_cpp.h>
#include <transfer/gfal_transfer.h>

#include <common/gfal_lib_test.h>
#include <common/gfal_gtest_asserts.h>


void transfer_callback(const gfalt_event_t e, gpointer user_data);

#define NBPAIRS 3


class CopyBulk: public testing::Test {
public:
    static const char* source_root;
    static const char* destination_root;

    char *sources[NBPAIRS];
    char *destinations[NBPAIRS];
    size_t done;

    gfal2_context_t handle;
    gfalt_params_t params;

    char original_checksum[32];

    CopyBulk(): done(0) {
        GError *error = NULL;
        handle =  gfal2_context_new(&error);
        Gfal::gerror_to_cpp(&error);
        params = gfalt_params_handle_new(NULL);
        gfalt_set_checksum_check(params, TRUE, NULL);
        gfalt_set_event_callback(params, transfer_callback, NULL);
        gfalt_set_user_data(params, this, NULL);

        for (size_t i = 0; i < NBPAIRS; ++i) {
            sources[i] = new char[2048];
            destinations[i] = new char[2048];
        }
    }

    virtual ~CopyBulk() {
        gfal2_context_free(handle);
        gfalt_params_handle_delete(params, NULL);

        for (size_t i = 0; i < NBPAIRS; ++i) {
            delete [] sources[i];
            delete [] destinations[i];
        }
    }

    virtual void SetUp() {
        char source_base[2048];
        char dest_base[2048];
        int ret;
        GError* error = NULL;

        generate_random_uri(source_root, "copyfile_bulk_user_source", source_base, 2048);
        generate_random_uri(destination_root, "copyfile_bulk_user_destination", dest_base, 2048);

        for (size_t i = 0; i < NBPAIRS; ++i) {
            snprintf(sources[i], 2048, "%s_%zu", source_base, i);
            snprintf(destinations[i], 2048, "%s_%zu", dest_base, i);

            ret = generate_file_if_not_exists(handle, sources[i], "file:///etc/hosts", &error);
            EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, error);
        }

        ret = gfal2_checksum(handle, "file:///etc/hosts", "ADLER32", 0, 0,
                original_checksum, sizeof original_checksum, &error);
        EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, error);

        done = 0;

        gfalt_set_user_defined_checksum(params, NULL, NULL, NULL);
        gfalt_set_checksum_check(params, FALSE, NULL);
        gfalt_set_replace_existing_file(params, FALSE, NULL);
    }

    virtual void TearDown() {
        for (size_t i = 0; i < NBPAIRS; ++i) {
            gfal_unlink(sources[i]);
            gfal_unlink(destinations[i]);
        }
    }
};

const char* CopyBulk::source_root;
const char* CopyBulk::destination_root;


void transfer_callback(const gfalt_event_t e, gpointer user_data)
{
    CopyBulk* copy = static_cast<CopyBulk*>(user_data);

    const char* stage = g_quark_to_string(e->stage);
    const char* side;
    switch (e->side) {
        case GFAL_EVENT_SOURCE:
            side = "SOURCE";
            break;
        case GFAL_EVENT_DESTINATION:
            side = "DESTINATION";
            break;
        default:
            side = "BOTH";
    }

    printf("%-15s %-15s %s\n", side, stage, e->description);

    if (e->stage == GFAL_EVENT_TRANSFER_EXIT) {
        copy->done++;
    }
}


TEST_F(CopyBulk, CopyBulk)
{
    GError* op_error = NULL;
    GError** file_errors = NULL;
    int ret = 0;

    ret = gfalt_copy_bulk(handle, params, NBPAIRS, sources, destinations, NULL, &op_error, &file_errors);
    EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, op_error);

    if (file_errors) {
        for (size_t i = 0; i < NBPAIRS; ++i) {
            if (file_errors[i]) {
                EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, file_errors[i]);
                g_error_free(file_errors[i]);
            }
        }
        g_free(file_errors);
    }

    if (op_error)
        g_error_free(op_error);

    ASSERT_EQ(NBPAIRS, done);

    // Do not trust! Make sure they do exist
    if (ret == 0) {
        struct stat st;
        GError* tmp_err = NULL;
        for (size_t i = 0; i < NBPAIRS; ++i) {
            ret = gfal2_stat(handle, destinations[i], &st, &tmp_err);
            EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, tmp_err);
            if (tmp_err)
                g_error_free(tmp_err);
            tmp_err = NULL;
        }
    }
}


TEST_F(CopyBulk, CopyBulkSomeFail)
{
    // Remove source for even transfers, they should fail
    int removed = 0;
    for (size_t i = 0; i < NBPAIRS; ++i) {
        if (i % 2 == 0) {
            GError* tmp_err = NULL;
            gfal2_unlink(handle, sources[i], &tmp_err);
            if (tmp_err)
                g_error_free(tmp_err);
            ++removed;
        }
    }

    GError* op_error = NULL;
    GError** file_errors = NULL;
    int ret = 0;

    ret = gfalt_copy_bulk(handle, params, NBPAIRS, sources, destinations, NULL, &op_error, &file_errors);
    ASSERT_LT(ret, 0);
    EXPECT_PRED_FORMAT2(AssertGfalSuccess, 0, op_error);
    ASSERT_NE((void*)NULL, file_errors);

    if (file_errors) {
        for (size_t i = 0; i < NBPAIRS; ++i) {
            if (i % 2 == 0) {
                EXPECT_PRED_FORMAT3(AssertGfalErrno, ret, file_errors[i], ENOENT);
            }
            else {
                EXPECT_PRED_FORMAT2(AssertGfalSuccess, 0, file_errors[i]);
            }
            if (file_errors[i])
                g_error_free(file_errors[i]);
        }
        g_free(file_errors);
    }

    if (op_error)
        g_error_free(op_error);
}


TEST_F(CopyBulk, CopyBulkChecksuming)
{
    const char *checksums[NBPAIRS] = {0};

    // All checksums fine, except first
    for (size_t i = 0; i < NBPAIRS; ++i)
        checksums[i] = original_checksum;
    checksums[0] = "0D3902E7";

    GError* op_error = NULL;
    GError** file_errors = NULL;
    int ret = 0;

    gfalt_set_user_defined_checksum(params, "ADLER32", NULL, NULL);
    gfalt_set_checksum_check(params, TRUE, NULL);
    ret = gfalt_copy_bulk(handle, params, NBPAIRS, sources, destinations, checksums, &op_error, &file_errors);

    ASSERT_LT(ret, 0);
    EXPECT_PRED_FORMAT2(AssertGfalSuccess, 0, op_error);
    ASSERT_NE((void*)NULL, file_errors);

    if (file_errors) {
        EXPECT_PRED_FORMAT3(AssertGfalErrno, ret, file_errors[0], EIO);
        for (size_t i = 1; i < NBPAIRS; ++i) {
            EXPECT_PRED_FORMAT2(AssertGfalSuccess, 0, file_errors[i]);
        }
        g_free(file_errors);
    }

    if (op_error)
        g_error_free(op_error);
}


TEST_F(CopyBulk, CopyDestinationExists)
{
    GError* error = NULL;
    int ret;

    for (size_t i = 0; i < NBPAIRS; ++i) {
        ret = generate_file_if_not_exists(handle, destinations[i], "file:///etc/hosts", &error);
        EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, error);
    }

    GError* op_error = NULL;
    GError** file_errors = NULL;

    ret = gfalt_copy_bulk(handle, params, NBPAIRS, sources, destinations, NULL, &op_error, &file_errors);
    ASSERT_LT(ret, 0);
    EXPECT_PRED_FORMAT2(AssertGfalSuccess, 0, op_error);

    if (file_errors) {
        for (size_t i = 0; i < NBPAIRS; ++i) {
            EXPECT_PRED_FORMAT3(AssertGfalErrno, ret, file_errors[i], EEXIST);
        }
        g_free(file_errors);
    }

    if (op_error)
        g_error_free(op_error);
}


TEST_F(CopyBulk, CopyOverwrite)
{
    GError* error = NULL;
    int ret;

    for (size_t i = 0; i < NBPAIRS; ++i) {
        ret = generate_file_if_not_exists(handle, destinations[i], "file:///etc/hosts", &error);
        EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, error);
    }

    GError* op_error = NULL;
    GError** file_errors = NULL;

    gfalt_set_replace_existing_file(params, TRUE, NULL);
    ret = gfalt_copy_bulk(handle, params, NBPAIRS, sources, destinations, NULL, &op_error, &file_errors);
    EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, op_error);

    if (file_errors) {
        for (size_t i = 0; i < NBPAIRS; ++i) {
            EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, file_errors[i]);
        }
        g_free(file_errors);
    }

    if (op_error)
        g_error_free(op_error);
}


int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);

    if (argc < 3) {
        printf("Missing source and destination base urls\n");
        printf("\t%s [options] gsiftp://host/base/path/ gsiftp://destination/base/path/\n", argv[0]);
        return 1;
    }

    CopyBulk::source_root = argv[1];
    CopyBulk::destination_root = argv[2];

    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-v") == 0)
            gfal_set_verbose(GFAL_VERBOSE_TRACE | GFAL_VERBOSE_VERBOSE | GFAL_VERBOSE_DEBUG);
    }

    return RUN_ALL_TESTS();
}
