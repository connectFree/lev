/*
 *  Copyright 2012 The Luvit Authors. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include "luv.h"
#include "uv.h"
#include <stdlib.h>
#include <string.h>
#include "uv-private/ev.h"

#include "luv_misc.h"

static const luaL_reg luv_f[] = {

  /* Misc functions */
  {"run", luv_run},
  {"printActiveHandles", luv_print_active_handles},
  {"printAllHandles", luv_print_all_handles},
  {"updateTime", luv_update_time},
  {"now", luv_now},
  {"hrtime", luv_hrtime},
  {"getFreeMemory", luv_get_free_memory},
  {"getTotalMemory", luv_get_total_memory},
  {"loadavg", luv_loadavg},
  {"uptime", luv_uptime},
  {"cpuInfo", luv_cpu_info},
  {"interfaceAddresses", luv_interface_addresses},
  {"execpath", luv_execpath},
  {"getProcessTitle", luv_get_process_title},
  {"setProcessTitle", luv_set_process_title},
  {"handleType", luv_handle_type},
  {"activateSignalHandler", luv_activate_signal_handler},
  {NULL, NULL}
};

/* When the lhandle is freed, do some helpful sanity checks */
static int luv_handle_gc(lua_State* L) {
  luv_handle_t* lhandle = (luv_handle_t*)lua_touserdata(L, 1);
/*  printf("__gc %s lhandle=%p handle=%p\n", lhandle->type, lhandle, lhandle->handle);*/
  /* If the handle is still there, they forgot to close */
  if (lhandle->handle) {
    fprintf(stderr, "WARNING: forgot to close %s lhandle=%p handle=%p\n", lhandle->type, lhandle, lhandle->handle);
    uv_close(lhandle->handle, luv_on_close);
  }
  return 0;
}

LUALIB_API int luaopen_uv_native (lua_State* L) {
  /* metatable for handle userdata types */
  /* It is it's own __index table to save space */
  luaL_newmetatable(L, "luv_handle");
  lua_pushcfunction(L, luv_handle_gc);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);

  /* Create a new exports table with functions and constants */
  lua_newtable (L);

  luaL_register(L, NULL, luv_f);
  lua_pushnumber(L, UV_VERSION_MAJOR);
  lua_setfield(L, -2, "VERSION_MAJOR");
  lua_pushnumber(L, UV_VERSION_MINOR);
  lua_setfield(L, -2, "VERSION_MINOR");

  return 1;
}

