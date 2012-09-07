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

/*
 * Error utility functions.
 */

static void fs_push_uv_error(lua_State *L, uv_err_t err) {
  lua_createtable(L, 0, 2);

  lua_pushstring(L, uv_strerror(err));
  lua_setfield(L, -2, "message");

  lua_pushstring(L, uv_err_name(err));
  lua_setfield(L, -2, "code");
}

static uv_err_t code_to_uv_error(uv_err_code errcode) {
  uv_err_t err;
  err.code = errcode;
  return err;
}

#define IS_ASYNC(req) (req)->data
#define UV_ERR(req) (IS_ASYNC(req) ? code_to_uv_error((req)->errorno) \
                                   : uv_last_error((req)->loop))

/*
 * callback utilities
 */

static int fs_checkcallback(lua_State *L, int index) {
  luaL_checktype(L, index, LUA_TFUNCTION);
  lua_pushvalue(L, index);
  return luaL_ref(L, LUA_REGISTRYINDEX);
}

static void fs_push_callback(lua_State *L, int ref) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, ref);

  /*
   * We can safely call luaL_unref() here because now the stack holds the value
   * so it is not garbage collected.
   */
  luaL_unref(L, LUA_REGISTRYINDEX, ref);
}

/*
 * fs request memory management
 */

static uv_fs_t *alloc_fs_rec() {
  MemBlock *block = lev_slab_getBlock(sizeof(uv_fs_t));
  lev_slab_incRef(block);
  return block->bytes;
}

static dispose_fs_rec(uv_fs_t *req) {
  uv_fs_req_cleanup(req);
  MemBlock* block = container_of(req, MemBlock, bytes);
  lev_slab_decRef(block);
}

/*
 * pushing results
 */

static int push_results(lua_State *L, uv_fs_t *req) {
  if (req->result == -1) {
    fs_push_uv_error(L, UV_ERR(req));
    return 1;
  }

  switch (req->fs_type) {
  case UV_FS_OPEN:
    lua_pushnil(L);
    lua_pushnumber(L, req->result);
    return 2;
  default:
    return 0;
  }
}

static void after_async_fs(uv_fs_t *req) {
  lua_State *L = req->loop->data;
  int ret_n;

  fs_push_callback(L, req->data);
  ret_n = push_results(L, req);
  lua_call(L, ret_n, 0);
  dispose_fs_rec(req);
}

static int after_sync_fs(lua_State* L, uv_fs_t *req) {
  int ret_n = push_results(L, req);
  dispose_fs_rec(req);
  return ret_n;
}

/*
 * fs.exists
 */

static int push_exists_results(lua_State *L, uv_fs_t *req) {
  lua_pushboolean(L, req->result != -1);
  return 1;
}

static void after_async_exists(uv_fs_t *req) {
  lua_State *L = req->loop->data;
  int ret_n;

  fs_push_callback(L, req->data);
  ret_n = push_exists_results(L, req);
  lua_call(L, ret_n, 0);
  dispose_fs_rec(req);
}

static int after_sync_exists(lua_State* L, uv_fs_t *req) {
  int ret_n = push_exists_results(L, req);
  dispose_fs_rec(req);
  return ret_n;
}

static int fs_exists(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_rec();
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = after_async_exists;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_stat(loop, req, path, cb);
  return after_sync_exists(L, req);
}

/*
 * fs.close
 */
static int fs_close(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_rec();
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  int fd = luaL_checkint(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = after_async_fs;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_close(loop, req, fd, cb);
  return after_sync_fs(L, req);
}

/*
 * fs.mkdir
 */
static int fs_mkdir(lua_State* L) {
  int mode;
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_rec();
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n && lua_isnumber(L, arg_i)) {
    mode = luaL_checkint(L, arg_i++);
  } else {
    mode = 0777;
  }
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = after_async_fs;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_mkdir(loop, req, path, mode, cb);
  return after_sync_fs(L, req);
}

/*
 * fs.rmdir
 */
static int fs_rmdir(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_rec();
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = after_async_fs;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_rmdir(loop, req, path, cb);
  return after_sync_fs(L, req);
}

/*
 * fs.unlink
 */
static int fs_unlink(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_rec();
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = after_async_fs;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_unlink(loop, req, path, cb);
  return after_sync_fs(L, req);
}

/*
 * fs.open
 */

typedef struct mode_mapping_s {
  char *name;
  int flags;
} mode_mapping_t;

static mode_mapping_t mode_mappings[] = {
   { "r",   O_RDONLY                      }
  ,{ "w",   O_WRONLY | O_CREAT | O_TRUNC  }
  ,{ "a",   O_WRONLY | O_CREAT | O_APPEND }
  ,{ "r+",  O_RDWR                        }
  ,{ "w+",  O_RDWR   | O_CREAT | O_TRUNC  }
  ,{ "a+",  O_RDWR   | O_CREAT | O_APPEND }
#ifndef _WIN32
  ,{ "rs",  O_RDONLY | O_SYNC             }
  ,{ "rs+", O_RDWR   | O_SYNC             }
#endif
  ,{ NULL,  0                             }
};

static int fs_to_flags(const char *flags_str) {
  mode_mapping_t *mapping;
  for (mapping = mode_mappings; mapping->name; mapping++) {
    if (strcmp(flags_str, mapping->name) == 0) {
      return mapping->flags;
    }
  }
  return -1;
}

static int fs_checkflags(lua_State *L, int index) {
  const char *flags_str = luaL_checkstring(L, index);
  int flags = fs_to_flags(flags_str);
  if (flags == -1) {
    return luaL_argerror(L, index, "Invalid flags");
  }
  return flags;
}

static int fs_open(lua_State* L) {
  int mode;
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_rec();
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  int flags = fs_checkflags(L, arg_i++);
  if (arg_i <= arg_n && lua_isnumber(L, arg_i)) {
    mode = luaL_checkint(L, arg_i++);
  } else {
    mode = 0666;
  }
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = after_async_fs;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_open(loop, req, path, flags, mode, cb);
  return after_sync_fs(L, req);
}

static luaL_reg functions[] = {
  { "close",      fs_close        }
 ,{ "exists",     fs_exists       }
 ,{ "open",       fs_open         }
 ,{ "mkdir",      fs_mkdir        }
 ,{ "rmdir",      fs_rmdir        }
 ,{ "unlink",     fs_unlink       }
 ,{ NULL,         NULL            }
};

void luaopen_lev_fs(lua_State *L) {
  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  lua_setfield(L, -2, "fs");
}
