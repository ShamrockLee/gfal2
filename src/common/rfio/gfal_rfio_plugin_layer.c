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


/*
 * @file gfal_rfio_plugin_layer.c
 * @brief file for the external call, abstraction layer for mock purpose
 * @author Devresse Adrien
 * @version 0.1
 * @date 30/06/2011
 * 
 **/


#include <regex.h>
#include <time.h> 
#include <dlfcn.h>
#include <stdlib.h>
#include <common/gfal_common_internal.h>
#include <common/gfal_common_errverbose.h>
#include <common/gfal_common_plugin.h>
#include <common/gfal_types.h>
#include "gfal_rfio_plugin_layer.h"

static char* libdpm_name= "libdpm.so.1";
static char* libcastor_name= "libshift.so.2.1";

struct rfio_proto_ops * gfal_rfio_internal_loader_base(GError** err){
	void *dlhandle=NULL;
	struct rfio_proto_ops * pops = NULL;
	GError* tmp_err=NULL;
	char *p;
	char* libname=NULL;
	p = getenv ("LCG_RFIO_TYPE");
	if (p && strcmp (p, "dpm") == 0) {
		libname = libdpm_name;
	} else if (p && strcmp (p, "castor") == 0) {
		libname= libcastor_name;
	}
	if( libname != NULL){
		gfal_log(GFAL_VERBOSE_VERBOSE, " lib rfio defined in LCG_RFIO_TYPE : %s", libname);
		if( (dlhandle = dlopen(libname, RTLD_LAZY)) == NULL){
			g_set_error(&tmp_err, 0, EPROTONOSUPPORT, " library %s for the rfio_plugin cannot be loaded properly, failure : %s ", libname, dlerror());
		}	
	}else{
		gfal_log(GFAL_VERBOSE_VERBOSE, " lib rfio is not defined in LCG_RFIO_TYPE, try to found it ");
		char* tab_lib[] = { libdpm_name, libcastor_name, NULL};
		char** p = tab_lib;
		while(*p != NULL){
			if((dlhandle = dlopen (*p, RTLD_LAZY)) != NULL){
				gfal_log(GFAL_VERBOSE_VERBOSE, "rfio library %s found  ! configured to us it", *p);
				break;	
			}
			p++;
		}
	    if(!dlhandle){	
			g_set_error(&tmp_err, 0, EPROTONOSUPPORT, " Unable to find %s or %s, failure : %s ", libcastor_name, libdpm_name, dlerror());
		}
	}
	if(dlhandle){
			pops = g_new0(struct rfio_proto_ops, 1);
			pops->geterror = (int (*) ()) dlsym (dlhandle, "rfio_serrno");
			pops->serror_r = (char* (*) (char*, size_t)) dlsym(dlhandle, "rfio_serror_r");
			pops->access = (int (*) (const char *, int)) dlsym (dlhandle, "rfio_access");
			pops->chmod = (int (*) (const char *, mode_t)) dlsym (dlhandle, "rfio_chmod");
			pops->close = (int (*) (int)) dlsym (dlhandle, "rfio_close");
			pops->closedir = (int (*) (DIR *)) dlsym (dlhandle, "rfio_closedir");
			pops->lseek = (off_t (*) (int, off_t, int)) dlsym (dlhandle, "rfio_lseek");
			pops->lseek64 = (off64_t (*) (int, off64_t, int)) dlsym (dlhandle, "rfio_lseek64");
			pops->lstat = (int (*) (const char *, struct stat *)) dlsym (dlhandle, "rfio_lstat");
			pops->lstat64 = (int (*) (const char *, struct stat64 *)) dlsym (dlhandle, "rfio_lstat64");
			pops->mkdir = (int (*) (const char *, mode_t)) dlsym (dlhandle, "rfio_mkdir");
			pops->open = (int (*) (const char *, int, ...)) dlsym (dlhandle, "rfio_open");
			pops->opendir = (DIR * (*) (const char *)) dlsym (dlhandle, "rfio_opendir");
			pops->read = (ssize_t (*) (int, void *, size_t)) dlsym (dlhandle, "rfio_read");
			pops->readdir = (struct dirent * (*) (DIR *)) dlsym (dlhandle, "rfio_readdir");
			pops->readdir64 = (struct dirent64 * (*) (DIR *)) dlsym (dlhandle, "rfio_readdir64");
			pops->rename = (int (*) (const char *, const char *)) dlsym (dlhandle, "rfio_rename");
			pops->rmdir = (int (*) (const char *)) dlsym (dlhandle, "rfio_rmdir");
			pops->setfilchg = (ssize_t (*) (int, const void *, size_t)) dlsym (dlhandle, "rfio_HsmIf_FirstWrite");
			pops->stat = (int (*) (const char *, struct stat *)) dlsym (dlhandle, "rfio_stat");
			pops->stat64 = (int (*) (const char *, struct stat64 *)) dlsym (dlhandle, "rfio_stat64");
			pops->unlink = (int (*) (const char *)) dlsym (dlhandle, "rfio_unlink");
			pops->write = (ssize_t (*) (int, const void *, size_t)) dlsym (dlhandle, "rfio_write");
	}
	if(tmp_err)
			g_propagate_prefixed_error(err, tmp_err, "[%s]", __func__);
	return pops;
	
}


struct rfio_proto_ops * (*gfal_rfio_internal_loader)(GError** err)= &gfal_rfio_internal_loader_base;

