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
#include <assert.h>
#include "luv_debug.h"

static char object_registry[0];

static void create_object_registry(lua_State* L) {
  lua_pushlightuserdata(L, object_registry);
  lua_newtable(L);

  lua_createtable(L, 0, 1);
  lua_pushstring(L, "v");
  lua_setfield(L, -2, "__mode");

  lua_setmetatable(L, -2);
  lua_rawset(L, LUA_REGISTRYINDEX);
}

void* new_object(lua_State* L, size_t size, const char* clazz) {
  void* object;

  /* Create object with metatable. */
  object = lua_newuserdata(L, size);
  luaL_getmetatable(L, clazz);
  lua_setmetatable(L, -2);

  /* Create storage for callbacks. */
  lua_createtable(L, 1, 0);
  lua_setfenv(L, -2);

  push_registry(L);

  /* Associate our object with the Lua object. */
  lua_pushlightuserdata(L, object);
  lua_pushvalue(L, -3); /* object */
  lua_rawset(L, -3); /* registry */

  /* Pop our object registry from the stack. */
  lua_pop(L, 1);

  return object;
}

void set_callback(lua_State* L, const char* name, int index) {
  index = abs_index(L, index);
  luaL_checktype(L, index, LUA_TFUNCTION);

  lua_getfenv(L, 1);
  lua_pushstring(L, name);
  lua_pushvalue(L, index);
  lua_rawset(L, -3);
  lua_pop(L, 1);
}


void clear_callback(lua_State* L, const char* name, void* object) {
  push_object(L, object);
  lua_getfenv(L, -1);
  lua_pushstring(L, name);
  lua_pushnil(L);
  lua_rawset(L, -3);
  lua_pop(L, 2);
}

void push_registry(lua_State* L) {
  /* Push our object registry onto the stack. */
  lua_pushlightuserdata(L, object_registry);
  lua_rawget(L, LUA_REGISTRYINDEX);
  assert(lua_istable(L, -1));
}

void push_object(lua_State* L, void* object) {
  /* Push our object registry onto the stack. */
  lua_pushlightuserdata(L, object_registry);
  lua_rawget(L, LUA_REGISTRYINDEX);
  assert(lua_istable(L, -1));

  /* Look up the Lua object associated with this handle. */
  lua_pushlightuserdata(L, object);
  lua_rawget(L, -2);

  /* STACK: <registry> <object> */
  lua_remove(L, -2);
  /* STACK: <object> */
}

int push_callback(lua_State* L, void* object, const char* name) {

  push_object(L, object);

  /* Get the callback table. */
  lua_getfenv(L, -1);

  assert(lua_istable(L, -1));

  /* Look up callback. */
  lua_pushstring(L, name);
  lua_rawget(L, -2);
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, -2); /* no callback registered; cleanup */
    return 0;
  }

  /* STACK: <object> <table> <callback> */
  lua_remove(L, -2);
  /* STACK: <object> <callback> */

  /* STACK: <object> <callback> */
  lua_pushvalue(L, -2);
  lua_remove(L, -3);
  /* STACK: <callback> <object> */
  return 1; /* OK */
}

#ifdef WIN32
__declspec(dllexport)
#endif
int luaopen_levbase(lua_State *L) {
  luaL_reg functions[] = {{NULL, NULL}};

  uv_default_loop()->data = L;
  create_object_registry(L);

  luaL_register(L, "levbase", functions);
  luaopen_lev_core(L);
  luaopen_lev_tcp(L);

  return 1;
}