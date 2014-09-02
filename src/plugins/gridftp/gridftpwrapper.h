#ifndef GRIDFTPWRAPPER_H
#define GRIDFTPWRAPPER_H
/*
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gfal_api.h>
#include <exceptions/gfalcoreexception.hpp>
#include <time_utils.h>

#include <ctime>
#include <algorithm>
#include <memory>

#include <glib.h>
#include <glibmm.h>

#include <globus_ftp_client.h>
#include <globus_gass_copy.h>


// Forward declarations
class GridFTPFactory;
class GridFTPSessionHandler;
class GridFTPSessionHandler;
class GridFTPRequestState;
class GridFTPStreamState;


enum GridFTPRequestStatus {
    GRIDFTP_REQUEST_NOT_LAUNCHED,
    GRIDFTP_REQUEST_RUNNING,
    GRIDFTP_REQUEST_FINISHED,
};

enum GridFTPRequestType {
    GRIDFTP_REQUEST_GASS, GRIDFTP_REQUEST_FTP
};



class GridFTPRequestState {
 public:
    GridFTPRequestState(GridFTPSessionHandler * s, GridFTPRequestType request_type = GRIDFTP_REQUEST_FTP);
	virtual ~GridFTPRequestState();

	void wait(const Glib::Quark &scope, time_t timeout = -1);
	void cancel(const Glib::Quark &scope, const std::string& msg);

	GridFTPSessionHandler* handler;
	GridFTPRequestType request_type;

    globus_mutex_t lock;
    globus_cond_t cond;
    Gfal::CoreException* error;
    bool done;

    time_t default_timeout;
};

class GridFTPStreamState: public GridFTPRequestState {
public:
    off_t offset; // file offset in the stream
	bool eof;     // end of file reached

	GridFTPStreamState(GridFTPSessionHandler * s);
    virtual ~GridFTPStreamState();
};


struct GassCopyAttrHandler {
    GassCopyAttrHandler(globus_ftp_client_operationattr_t* ftp_operation_attr);
    ~GassCopyAttrHandler();
    globus_gass_copy_attr_t attr_gass;
    globus_ftp_client_operationattr_t operation_attr_ftp_for_gass;
};


class GridFTPSession {
public:
    GridFTPSession(const std::string& hostname);
    ~GridFTPSession();

    std::string hostname;

    globus_ftp_client_handle_t handle_ftp;
    globus_ftp_client_plugin_t debug_ftp_plugin;
    globus_ftp_client_handleattr_t attr_handle;
    globus_ftp_client_operationattr_t operation_attr_ftp;
    globus_gass_copy_handle_t gass_handle;
    globus_gass_copy_handleattr_t gass_handle_attr;
    globus_ftp_control_dcau_t dcau_control;

    // options
    globus_ftp_control_parallelism_t parallelism;
    globus_ftp_control_mode_t mode;
    globus_ftp_control_tcpbuffer_t tcp_buffer_size;

    // Handy option setters
    void set_gridftpv2(bool v2);
    void set_ipv6(bool ipv6);
    void set_udt(bool udt);
    void set_dcau(bool dcau);
    void set_delayed_pass(bool enable);
    void set_nb_streams(unsigned int nbstreams);
    void set_tcp_buffer_size(guint64 tcp_buffer_size);
};


class GridFTPSessionHandler {
public:
    GridFTPSessionHandler(GridFTPFactory* f, const std::string &uri);
    ~GridFTPSessionHandler();

    globus_ftp_client_handle_t* get_ftp_client_handle();
    globus_gass_copy_handle_t* get_gass_copy_handle();
    globus_ftp_client_operationattr_t* get_ftp_client_operationattr();
    globus_gass_copy_handleattr_t* get_gass_copy_handleattr();
    globus_ftp_client_handleattr_t* get_ftp_client_handleattr();

    GridFTPFactory* get_factory();

    void disable_reuse();

    GridFTPSession* session;

private:
    bool _isDirty;

    GridFTPFactory* factory;
    std::string hostname;
};


class GridFTPFactory {
public:
    GridFTPFactory(gfal2_context_t handle);
    ~GridFTPFactory();

    /** Get a suitable session, new or reused
     **/
    GridFTPSession* get_session(const std::string &hostname);

    /** Release the session, and close it if should not be reused
     * If destroy is true, the session will NOT be reused even if session reuse is configured
     **/
    void release_session(GridFTPSession* h, bool destroy = false);

    gfal2_context_t get_gfal2_context();

private:
    gfal2_context_t gfal2_context;
    // session re-use management
    bool session_reuse;
    unsigned int size_cache;
    // session cache
    std::multimap<std::string, GridFTPSession*> session_cache;
    Glib::Mutex mux_cache;

    void recycle_session(GridFTPSession* sess);
    void clear_cache();
    GridFTPSession* get_recycled_handle(const std::string &hostname);
    GridFTPSession* get_new_handle(const std::string &hostname);
};


void globus_ftp_client_done_callback(void * user_arg,
        globus_ftp_client_handle_t * handle, globus_object_t * error);

void globus_gass_client_done_callback(
        void * callback_arg,
        globus_gass_copy_handle_t * handle,
        globus_object_t * error);

// do atomic read operation from globus async call
ssize_t gridftp_read_stream(const Glib::Quark & scope,
        GridFTPStreamState* stream, void* buffer, size_t s_read);

// do atomic write operation from globus async call
ssize_t gridftp_write_stream(const Glib::Quark & scope,
        GridFTPStreamState* stream, const void* buffer, size_t s_write,
        bool eof);


// return 0 if no error, or return errno and set the error string properly
// error allocation is dynamic
int gfal_globus_error_convert(globus_object_t * error, char ** str_error);

// throw Glib::Error if error associated with this result
void gfal_globus_check_result(const Glib::Quark & scope, globus_result_t res);

void gfal_globus_set_credentials(gfal2_context_t, globus_ftp_client_operationattr_t*);

void gfal_globus_set_credentials(const char* ucert, const char* ukey, globus_ftp_client_operationattr_t*);

#endif /* GRIDFTPWRAPPER_H */
