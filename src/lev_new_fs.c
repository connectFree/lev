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

  X:TODO Beginning of FS Module...
  
*/

#define UNWRAP(h) \
  fs_obj* self = container_of((h), fs_obj, handle); \
  lua_State* L = self->handle.loop->data;

typedef struct {
  LEVBASE_REF_FIELDS
} fs_obj;

static int fs_new(lua_State* L) {
  uv_loop_t* loop;
  fs_obj* self;

  loop = uv_default_loop();
  assert(L == loop->data);

  self = (fs_obj*)create_obj_init_ref(L, sizeof *self, "lev.fs");

  return 1;
}

static luaL_reg methods[] = {
   /*{ "method_name",     ...      }*/
  { NULL,         NULL            }
};


static luaL_reg functions[] = {
   { "new", fs_new }
  ,{ NULL, NULL }
};


void luaopen_lev_fs(lua_State *L) {
  luaL_newmetatable(L, "lev.fs");
  luaL_register(L, NULL, methods);
  lua_setfield(L, -1, "__index");

  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  lua_setfield(L, -2, "fs");
}