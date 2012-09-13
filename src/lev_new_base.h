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

#ifndef LEVBASE_H_
#define LEVBASE_H_

#include "uv.h"

#include <stddef.h> /* offsetof */

#include <lua.h>
#include <lauxlib.h>

#include "lev_slab.h"

/* from Lua source */
#define abs_index(L, i) \
  ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : lua_gettop(L) + (i) + 1)

#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))

#define ARRAY_SIZE(a) (sizeof((a)) / sizeof((a)[0]))

typedef struct _LevRefStruct {
  LEVBASE_REF_FIELDS
} LevRefStruct_t;

void* create_obj_init_ref(lua_State* L, size_t size, const char *class_name);
void lev_handle_ref(lua_State* L, LevRefStruct_t* lhandle, int index);
void lev_handle_unref(lua_State* L, LevRefStruct_t* lhandle);

int luaopen_levbase(lua_State *L);

void luaopen_lev_fs(lua_State *L); /* lev.fs */
void luaopen_lev_dns(lua_State *L); /* lev.dns */
void luaopen_lev_tcp(lua_State *L); /* lev.tcp */
void luaopen_lev_udp(lua_State *L); /* lev.udp */
void luaopen_lev_core(lua_State *L); /* lev.core */
void luaopen_lev_pipe(lua_State *L); /* lev.pipe */
void luaopen_lev_mpack(lua_State *L); /* lev.mpack */
void luaopen_lev_timer(lua_State *L); /* lev.timer */
void luaopen_lev_buffer(lua_State *L); /* lev.buffer */
void luaopen_lev_process(lua_State *L); /* lev.process */

/* buffer helper functions */
int lev_pushbuffer_from_mb(lua_State *L, MemBlock *mb, size_t until, unsigned char *slice);
uv_buf_t lev_buffer_to_uv(lua_State *L, int index);
MemSlice * lev_buffer_new(lua_State *L, size_t size, const char *temp, size_t temp_size);
#define lev_checkbuffer(L, index) \
    ((MemSlice *)luaL_checkudata((L), (index), "lev.buffer"))

void* new_object(lua_State* L, size_t size, const char* clazz);
void set_callback(lua_State* L, const char* name, int index);
void clear_callback(lua_State* L, const char* name, void* object);
void push_registry(lua_State* L);
void push_object(lua_State* L, void* object);

int _push_callback(lua_State* L, void* object, const char* name, int pop_object);
/* push regular callback (with object */
#define push_callback(L, object, name)  _push_callback(L, object, name, 0)
/* push callback without object */
#define push_callback_no_obj(L, object, name)  _push_callback(L, object, name, 1)

/* Loop register/retrive functions */
void lev_set_loop(lua_State *L, uv_loop_t *loop);
uv_loop_t* lev_get_loop(lua_State *L);

/* error helper function */
void lev_push_uv_err(lua_State *L, uv_err_t err);
uv_err_t lev_code_to_uv_err(uv_err_code errcode);

/* request helper macros */
#define LEV_IS_ASYNC_REQ(req) ((req)->cb)
#define LEV_UV_ERR_FROM_REQ(req) \
    (LEV_IS_ASYNC_REQ(req) ? lev_code_to_uv_err((req)->errorno) \
                           : uv_last_error((req)->loop))

#define LEV_SET_FIELD(name, type, val) \
  lua_push##type(L, val);              \
  lua_setfield(L, -2, #name)

#endif /* LEVBASE_H_ */
