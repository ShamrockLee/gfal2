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
 * gfal_common_all.c
 * core file for the utility function of gfal common part
 * author Devresse Adrien
 * */

#include <dlfcn.h>
#include <regex.h>
#include <stdlib.h>



#include <common/gfal_prototypes.h>
#include <common/gfal_types.h>
#include <common/gfal_common_plugin.h>
#include <common/gfal_common_errverbose.h>
#include <common/gfal_common_dir_handle.h>
#include <common/gfal_common_file_handle.h>
#include <config/gfal_config_internal.h>

#define XVERSION_STR(x) #x
#define VERSION_STR(x) XVERSION_STR(x)

/* the version should be set by a "define" at the makefile level */
static const char *gfalversion = VERSION_STR(VERSION);



// initialization
__attribute__((constructor))
void core_init(){
    if (!g_thread_supported())
      g_thread_init(NULL);
}




 /*
 * initiate a gfal's context with default parameters for use
 */
gfal_handle gfal_initG (GError** err)
{
	GError* tmp_err=NULL;
	gfal_handle handle = g_new0(struct gfal_handle_,1);// clear allocation of the struct and set defautl options
	if(handle == NULL){
		errno= ENOMEM;
		g_set_error(err,0,ENOMEM, "[gfal_initG] bad allocation, no more memory free");
		return NULL;
	}
	handle->plugin_opt.plugin_number= 0;
	
    handle->conf = gfal_conf_new(&tmp_err);
    if(!tmp_err){
        gfal_plugins_instance(handle, &tmp_err); // load and instanciate all the plugins
    }
    if(tmp_err){
        g_free(handle);
        handle = NULL;
    }
	
	if(tmp_err)
		g_propagate_prefixed_error(err, tmp_err, "[%s]", __func__);
	return handle;
}

//free a gfal's handle, safe if null
void gfal_handle_freeG (gfal_handle handle){
	if(handle == NULL)
		return;
	gfal_plugins_delete(handle, NULL);
	gfal_dir_handle_container_delete(&(handle->fdescs));
	gfal_conf_delete(handle->conf);
    g_list_free(handle->plugin_opt.sorted_plugin);
	g_free(handle);
	handle = NULL;
}



//return a string of the current gfal version
char *gfal_version(){
    return (char*) gfalversion;
}

 

 


