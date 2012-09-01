/*
 *  Copyright 2012 The lev Authors. All Rights Reserved.
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

#include "lev_new_base.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <lua.h>
#include <lauxlib.h>

static uv_timer_t gc_timer;

static void timer_gc_cb(uv_handle_t* handle) {
  lua_gc((lua_State *)handle->data, LUA_GCCOLLECT, 0);
}

static int core_run(lua_State* L) {
  uv_loop_t* loop;
  int r;

  loop = uv_default_loop();
  assert(L == loop->data);

  /* register gc timer*/
  r = uv_timer_init(uv_default_loop(), &gc_timer);
  gc_timer.data = L;
  r = uv_timer_start(&gc_timer, (uv_timer_cb)timer_gc_cb, 5000, 5000);


  r = uv_run(loop);
  lua_pushinteger(L, r);

  return 1;
}


static luaL_reg functions[] = {
  { "run", core_run },
  { NULL, NULL }
};


void luaopen_lev_core(lua_State *L) {
  luaL_register(L, NULL, functions);
}