#pragma once
#ifndef GRIFTP_IFCE_FILECOPY_H
#define GRIFTP_IFCE_FILECOPY_H
/*
* Copyright @ Members of the EMI Collaboration, 2010.
* See www.eu-emi.eu for details on the copyright holders.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "gridftpmodule.h"
#include "gridftpwrapper.h"

extern "C" int gridftp_plugin_filecopy(plugin_handle handle,
        gfal2_context_t context, gfalt_params_t params, const char* src,
        const char* dst, GError ** err);

extern "C" int gridftp_bulk_copy(plugin_handle plugin_data, gfal2_context_t context, gfalt_params_t params,
        size_t nbfiles, const char* const * srcs, const char* const * dsts,
        const char* const * checksums, GError** op_error, GError*** file_errors);

int gridftp_filecopy_delete_existing(GridFTPModule* module,
        gfalt_params_t params, const char * url);

void gridftp_create_parent_copy(GridFTPModule* module, gfalt_params_t params,
        const char * gridftp_url);

std::string return_hostname(const std::string &uri, gboolean use_ipv6);

#endif /* GRIFTP_IFCE_FILECOPY_H */
