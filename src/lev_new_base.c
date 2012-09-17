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
static MemBlock *_static_mb = NULL;

uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size) {
  uv_buf_t buf;
  size_t remaining;

  if (!_static_mb) {
    _static_mb = lev_slab_getBlock( suggested_size );
    lev_slab_incRef( _static_mb );
  }

  remaining = _static_mb->size - _static_mb->nbytes;
  if (_static_mb->size - _static_mb->nbytes < 512) {
    lev_slab_decRef( _static_mb );
    _static_mb = lev_slab_getBlock( suggested_size );
    lev_slab_incRef( _static_mb );
    remaining = _static_mb->size - _static_mb->nbytes;
  }

  buf.base = (char *)(_static_mb->bytes + _static_mb->nbytes);
  buf.len = remaining;

  return buf;
}

void lev_pushbuffer_from_static_mb(lua_State *L, int nread) {
  /* push new cBuffer */
  lev_pushbuffer_from_mb(
       L
      ,_static_mb
      ,nread
      ,_static_mb->bytes + _static_mb->nbytes
    ); /* automatically incRef's mb */
  _static_mb->nbytes += nread; /* consume nread bytes */
}

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

int _push_callback(lua_State* L, void* object, const char* name, int pop_object) {

  push_object(L, object);

  /* Get the callback table. */
  lua_getfenv(L, -1);
/*
  printf("push_callback: %s (%d)\n", name, lua_type(L, (-1)) );
  luv_lua_debug_stackdump(L, "push_callback");
*/
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
  lua_insert(L, -2);
  /* STACK: <callback> <object> */

  if (pop_object) {
    lua_pop(L, 1);
    /* STACK: <callback> */
  }

  return 1; /* OK */
}

void* create_obj_init_ref(lua_State* L, size_t size, const char *class_name) {
  LevRefStruct_t* self;
  lua_State* mainthread;

  self = new_object(L, size, class_name);

  self->refCount = 0;
 
  /* if handle create in a coroutine, we need hold the coroutine */
  mainthread = luv_get_main_thread(L);
  if (L != mainthread) { 
    lua_pushthread(L);
    self->threadref = luaL_ref(L, LUA_REGISTRYINDEX);
  } else {
    self->threadref = LUA_NOREF;
  }
  self->ref = LUA_NOREF;

  return (void *)self;

}


/* This needs to be called when an async function is started on a lhandle. */
void lev_handle_ref(lua_State* L, LevRefStruct_t* lhandle, int index) {
  /*printf("handle_ref\t %p:%p\n", lhandle, &lhandle->handle);*/
  /* If it's inactive, store a ref. */
  if (!lhandle->refCount) {
    lua_pushvalue(L, index);
    lhandle->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    /*printf("makeStrong\t lhandle=%p handle=%p\n", lhandle, &lhandle->handle);*/
  }
  lhandle->refCount++;
}

/* This needs to be called when an async callback fires on a lhandle. */
void lev_handle_unref(lua_State* L, LevRefStruct_t* lhandle) {
  lhandle->refCount--;
  assert(lhandle->refCount >= 0);
  /* If it's now inactive, clear the ref */
  if (!lhandle->refCount) {
    luaL_unref(L, LUA_REGISTRYINDEX, lhandle->ref);
    if (lhandle->threadref != LUA_NOREF) {
      luaL_unref(L, LUA_REGISTRYINDEX, lhandle->threadref);
      lhandle->threadref = LUA_NOREF;
    }
    lhandle->ref = LUA_NOREF;
    /*printf("handle_unref\t lhandle=%p handle=%p\n", lhandle, &lhandle->handle);*/
  }
}

/*
 * Loop register/retrive functions.
 */

void lev_set_loop(lua_State *L, uv_loop_t *loop) {
  lua_pushlightuserdata(L, loop);
  lua_setfield(L, LUA_REGISTRYINDEX, "lev.loop");
}

uv_loop_t* lev_get_loop(lua_State *L) {
  uv_loop_t *loop;
  lua_getfield(L, LUA_REGISTRYINDEX, "lev.loop");
  loop = lua_touserdata(L, -1);
  lua_pop(L, 1);
  return loop;
}

/*
 * Error helper functions.
 */

#define LEV_UV_ERR_CASE_GEN(val, name, s) \
  case val: return #name;

const char *lev_uv_errname(uv_err_code errcode) {
  switch (errcode) {
  UV_ERRNO_MAP(LEV_UV_ERR_CASE_GEN)
  default: return "UNKNOWN";
  }
}

#ifdef __APPLE__
#include <crt_externs.h>
/**
 * If compiled as a shared library, you do not have access to the environ symbol,
 * See man (7) environ for the fun details.
 */
char **lev_os_environ() { return *_NSGetEnviron(); }

#else

extern char **environ;

char **lev_os_environ() { return environ; }
#endif

#ifdef WIN32
__declspec(dllexport)
#endif
int luaopen_levbase(lua_State *L) {
  luaL_reg functions[] = {{NULL, NULL}};

  lev_get_loop(L)->data = L;
  create_object_registry(L);

  lev_slab_fill();

  luaL_register(L, "lev", functions);
  luaopen_lev_fs(L); /* lev.fs */
  luaopen_lev_dns(L); /* lev.dns */
  luaopen_lev_tcp(L); /* lev.tcp */
  luaopen_lev_udp(L); /* lev.udp */
  luaopen_lev_core(L); /* lev.core */
  luaopen_lev_pipe(L); /* lev.pipe */
  luaopen_lev_mpack(L); /* lev.mpack */
  luaopen_lev_timer(L); /* lev.timer */
  luaopen_lev_buffer(L); /* lev.buffer */
  luaopen_lev_process(L); /* lev.process */
  luaopen_lev_signal(L); /* lev.signal */

  return 1;
}
