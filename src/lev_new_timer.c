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
#include "luv_debug.h"

/*

  X:TODO Beginning of TIMER Module...
  
*/

#define UNWRAP(h) \
  timer_obj* self = container_of((h), timer_obj, handle); \
  lua_State* L = self->handle.loop->data;

typedef struct {
  LEVBASE_REF_FIELDS
  uv_timer_t handle;
} timer_obj;

static int timer_new(lua_State* L) {
  uv_loop_t* loop;
  timer_obj* self;
  int r;

  loop = lev_get_loop(L);
  assert(L == loop->data);

  self = (timer_obj*)create_obj_init_ref(L, sizeof *self, "lev.timer");
  r = uv_timer_init(loop, &self->handle);
  assert(r == 0);

  return 1;
}

static void timer_on_close(uv_handle_t *handle) {
  UNWRAP(handle);
  if (push_callback(L, self, "on_close")) {
    lua_call(L, 1, 0);/*, -3*/
    
  }
  lev_handle_unref(L, (LevRefStruct_t*)self);
}

static int timer_close(lua_State* L) {
  timer_obj *self;
  uv_timer_t *handle;
  int r;

  self = luaL_checkudata(L, 1, "lev.timer");

  if (lua_isfunction(L, 2))
    set_callback(L, "on_close", 2);

  handle = &self->handle;
  r = uv_timer_stop(handle);
  assert(r == 0);

  uv_close((uv_handle_t *)handle, timer_on_close);

  return 0;
}

static void on_timer(uv_timer_t *handle, int status) {
  UNWRAP(handle);
  push_callback(L, self, "on_timer");
  lua_pushinteger(L, status);
  lua_call(L, 2, 0);/*, -4*/
}

static int timer_start(lua_State* L) {
  timer_obj *self;
  long timeout;
  long repeat;
  int r;

  self = luaL_checkudata(L, 1, "lev.timer");
  set_callback(L, "on_timer", 2);
  timeout = luaL_optlong(L, 3, 0);
  repeat = luaL_optlong(L, 4, 0);

  r = uv_timer_start(&self->handle, on_timer, timeout, repeat);
  assert(r == 0);

  lev_handle_ref(L, (LevRefStruct_t*)self, 1);

  return 0;
}

static int timer_stop(lua_State* L) {
  timer_obj *self;
  int r;

  self = luaL_checkudata(L, 1, "lev.timer");
  r = uv_timer_stop(&self->handle);
  assert(r == 0);

  return 0;
}

static int timer_again(lua_State* L) {
  timer_obj *self;
  int r;

  self = luaL_checkudata(L, 1, "lev.timer");
  r = uv_timer_again(&self->handle);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(lev_get_loop(L)));
    return 1;
  }

  return 0;
}

static luaL_reg methods[] = {
  { "again",      timer_again     }
 ,{ "close",      timer_close     }
 ,{ "start",      timer_start     }
 ,{ "stop",       timer_stop      }
 ,{ NULL,         NULL            }
};


static luaL_reg functions[] = {
   { "new", timer_new }
  ,{ NULL, NULL }
};


void luaopen_lev_timer(lua_State *L) {
  luaL_newmetatable(L, "lev.timer");
  luaL_register(L, NULL, methods);
  lua_setfield(L, -1, "__index");

  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  lua_setfield(L, -2, "timer");
}
