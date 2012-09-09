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

#define IS_ASYNC(req) ((req)->cb)
#define UV_ERR(req) (IS_ASYNC(req) ? code_to_uv_error((req)->errorno) \
                                   : uv_last_error((req)->loop))

/*
 * argument utilities
 */
#define lev_checkbuffer(L, index) \
    ((MemSlice *)luaL_checkudata((L), (index), "lev.buffer"))

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

#ifdef MANAGE_REQ_IN_LUA
typedef struct fs_req_holder_s {
  int ref;
  uv_fs_t req;
} fs_req_holder_t;
#endif

static uv_fs_t *alloc_fs_req(lua_State *L) {
#ifdef MANAGE_REQ_IN_LUA
  fs_req_holder_t *holder = lua_newuserdata(L, sizeof(fs_req_holder_t));
  /* To avoid being gc'ed, we put this to registry. */
  holder->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  return &holder->req;
#else
  MemBlock *block = lev_slab_getBlock(sizeof(uv_fs_t));
  lev_slab_incRef(block);
  return block->bytes;
#endif
}

static dispose_fs_req(uv_fs_t *req) {
  uv_fs_req_cleanup(req);
#ifdef MANAGE_REQ_IN_LUA
  fs_req_holder_t *holder = container_of(req, fs_req_holder_t, req);
  lua_State *L = req->loop->data;
  luaL_unref(L, LUA_REGISTRYINDEX, holder->ref);
#else
  MemBlock* block = container_of(req, MemBlock, bytes);
  lev_slab_decRef(block);
#endif
}


/******************************************************************************/

#define FS_STAT_TNAME "lev.fs.stat"
#define lev_checkfsstat(L, idx) \
    ((uv_statbuf_t *)luaL_checkudata((L), (idx), FS_STAT_TNAME))

static int lev_pushtimespec(lua_State *L, struct timespec *timespec) {
  lua_pushnumber(L, timespec->tv_sec + timespec->tv_nsec / 1e9);
  return 1;
}

static int fs_stat_atime(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lev_pushtimespec(L, &buf->st_atime);
  return 1;
}

static int fs_stat_ctime(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lev_pushtimespec(L, &buf->st_ctime);
  return 1;
}

static int fs_stat_mtime(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lev_pushtimespec(L, &buf->st_mtime);
  return 1;
}


static int fs_stat_is_block_device(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushboolean(L, (buf->st_mode & S_IFMT) == S_IFBLK);
  return 1;
}

static int fs_stat_is_character_device(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushboolean(L, (buf->st_mode & S_IFMT) == S_IFCHR);
  return 1;
}

static int fs_stat_is_directory(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushboolean(L, (buf->st_mode & S_IFMT) == S_IFDIR);
  return 1;
}

static int fs_stat_is_fifo(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushboolean(L, (buf->st_mode & S_IFMT) == S_IFIFO);
  return 1;
}

static int fs_stat_is_file(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushboolean(L, (buf->st_mode & S_IFMT) == S_IFREG);
  return 1;
}

static int fs_stat_is_socket(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushboolean(L, (buf->st_mode & S_IFMT) == S_IFSOCK);
  return 1;
}

static int fs_stat_is_symbolic_link(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushboolean(L, (buf->st_mode & S_IFMT) == S_IFLNK);
  return 1;
}

static int fs_stat_blksize(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushnumber(L, buf->st_blksize);
  return 1;
}

static int fs_stat_blocks(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushnumber(L, buf->st_blocks);
  return 1;
}

static int fs_stat_dev(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushnumber(L, buf->st_dev);
  return 1;
}

static int fs_stat_gid(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushnumber(L, buf->st_gid);
  return 1;
}

static int fs_stat_ino(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushnumber(L, buf->st_ino);
  return 1;
}

static int fs_stat_mode(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushnumber(L, buf->st_mode);
  return 1;
}

static int fs_stat_nlink(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushnumber(L, buf->st_nlink);
  return 1;
}

static int fs_stat_rdev(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushnumber(L, buf->st_rdev);
  return 1;
}

static int fs_stat_size(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushnumber(L, buf->st_size);
  return 1;
}

static int fs_stat_uid(lua_State *L) {
  uv_statbuf_t *buf = lev_checkfsstat(L, 1);
  lua_pushnumber(L, buf->st_uid);
  return 1;
}

static luaL_reg fs_stat_methods[] = {
   {"atime", fs_stat_atime}
  ,{"blksize", fs_stat_blksize}
  ,{"blocks", fs_stat_blocks}
  ,{"ctime", fs_stat_ctime}
  ,{"dev", fs_stat_dev}
  ,{"gid", fs_stat_gid}
  ,{"ino", fs_stat_ino}
  ,{"is_block_device", fs_stat_is_block_device}
  ,{"is_character_device", fs_stat_is_character_device}
  ,{"is_directory", fs_stat_is_directory}
  ,{"is_fifo", fs_stat_is_fifo}
  ,{"is_file", fs_stat_is_file}
  ,{"is_socket", fs_stat_is_socket}
  ,{"is_symbolic_link", fs_stat_is_symbolic_link}
  ,{"mode", fs_stat_mode}
  ,{"mtime", fs_stat_mtime}
  ,{"nlink", fs_stat_nlink}
  ,{"rdev", fs_stat_rdev}
  ,{"size", fs_stat_size}
  ,{"uid", fs_stat_uid}
  ,{NULL, NULL}
};

int luaopen_lev_fs_stat(lua_State *L) {
  luaL_newmetatable(L, FS_STAT_TNAME);
  luaL_register(L, NULL, fs_stat_methods);
  lua_setfield(L, -1, "__index");

  return 1;
}

/******************************************************************************/

/*
 * pushing results
 *
 * NOTE: write returns err, written_byte_count, which is different from
 *       Node.js (err, written_byte_count, buffer)
 */

static int push_fs_stat(lua_State *L, uv_statbuf_t *buf) {
  uv_statbuf_t *userdata = lua_newuserdata(L, sizeof(uv_statbuf_t));
  *userdata = *buf;
  luaL_getmetatable(L, FS_STAT_TNAME);
  lua_setmetatable(L, -2);
  return 1;
}

static int push_readdir_results(lua_State *L, int entry_count,
    const char *entries) {
  const char *p;
  const char *q;
  int i;
  lua_createtable(L, entry_count, 0);
  for (i = 1, p = entries; i <= entry_count; ++i, p = q + 1) {
    q = strchr(p, '\0');
    assert(q != NULL);

    lua_pushlstring(L, p, q - p);
    lua_rawseti(L, -2, i);
  }
  return 1;
}

static int push_results(lua_State *L, uv_fs_t *req) {
  if (req->result == -1) {
    fs_push_uv_error(L, UV_ERR(req));
    return 1;
  }

  switch (req->fs_type) {
  case UV_FS_OPEN:
  case UV_FS_READ:
  case UV_FS_WRITE:
    lua_pushnil(L);
    lua_pushnumber(L, req->result);
    return 2;
  case UV_FS_LSTAT:
  case UV_FS_FSTAT:
  case UV_FS_STAT:
    lua_pushnil(L);
    push_fs_stat(L, req->ptr);
    return 2;
  case UV_FS_READLINK:
    lua_pushnil(L);
    lua_pushstring(L, req->ptr);
    return 2;
  case UV_FS_READDIR:
    lua_pushnil(L);
    push_readdir_results(L, req->result, req->ptr);
    return 2;
  default:
    return 0;
  }
}

static void on_fs_callback(uv_fs_t *req) {
  lua_State *L = req->loop->data;
  int ret_n;

  fs_push_callback(L, req->data);
  ret_n = push_results(L, req);
  lua_call(L, ret_n, 0);
  dispose_fs_req(req);
}

static int fs_post_handling(lua_State* L, uv_fs_t *req) {
  int ret_n = push_results(L, req);
  if (!IS_ASYNC(req)) {
    dispose_fs_req(req);
  }
  return ret_n;
}

/*
 * fs.exists
 */

static int push_exists_results(lua_State *L, uv_fs_t *req) {
  lua_pushboolean(L, req->result != -1);
  return 1;
}

static void on_exists_callback(uv_fs_t *req) {
  lua_State *L = req->loop->data;
  int ret_n;

  fs_push_callback(L, req->data);
  ret_n = push_exists_results(L, req);
  lua_call(L, ret_n, 0);
  dispose_fs_req(req);
}

static int exists_post_handling(lua_State* L, uv_fs_t *req) {
  int ret_n = push_exists_results(L, req);
  if (!IS_ASYNC(req)) {
    dispose_fs_req(req);
  }
  return ret_n;
}

static int fs_exists(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_exists_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_stat(loop, req, path, cb);
  return exists_post_handling(L, req);
}

/*
 * fs.close
 */
static int fs_close(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  int fd = luaL_checkint(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_close(loop, req, fd, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.chmod
 */
static int fs_chmod(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  int mode = luaL_checkint(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_chmod(loop, req, path, mode, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.chown
 */
static int fs_chown(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  int uid = luaL_checkint(L, arg_i++);
  int gid = luaL_checkint(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_chown(loop, req, path, uid, gid, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.fchmod
 */
static int fs_fchmod(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  int fd = luaL_checkint(L, arg_i++);
  int mode = luaL_checkint(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_fchmod(loop, req, fd, mode, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.fchown
 */
static int fs_fchown(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  int fd = luaL_checkint(L, arg_i++);
  int uid = luaL_checkint(L, arg_i++);
  int gid = luaL_checkint(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_fchown(loop, req, fd, uid, gid, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.fdatasync
 */
static int fs_fdatasync(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  int fd = luaL_checkint(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_fdatasync(loop, req, fd, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.fsync
 */
static int fs_fsync(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  int fd = luaL_checkint(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_fsync(loop, req, fd, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.futime
 */
static int fs_futime(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  int fd = luaL_checkint(L, arg_i++);
  lua_Number atime = luaL_checknumber(L, arg_i++);
  lua_Number mtime = luaL_checknumber(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_futime(loop, req, fd, atime, mtime, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.mkdir
 */
static int fs_mkdir(lua_State* L) {
  int mode;
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
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
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_mkdir(loop, req, path, mode, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.read
 */
static int fs_read(lua_State* L) {
  long file_pos;
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  int fd = luaL_checkint(L, arg_i++);
  MemSlice *ms = lev_checkbuffer(L, arg_i++);
  if (arg_i <= arg_n && lua_isnumber(L, arg_i)) {
    file_pos = luaL_checklong(L, arg_i++);
  } else {
    file_pos = -1;
  }
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_read(loop, req, fd, ms->slice, ms->until, file_pos, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.write
 */
static int fs_write(lua_State* L) {
  long file_pos;
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  int fd = luaL_checkint(L, arg_i++);
  MemSlice *ms = lev_checkbuffer(L, arg_i++);
  if (arg_i <= arg_n && lua_isnumber(L, arg_i)) {
    file_pos = luaL_checklong(L, arg_i++);
  } else {
    file_pos = -1;
  }
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_write(loop, req, fd, ms->slice, ms->until, file_pos, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.ftruncate
 */
static int fs_ftruncate(lua_State* L) {
  long file_size;
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  int fd = luaL_checkint(L, arg_i++);
  if (arg_i <= arg_n && lua_isnumber(L, arg_i)) {
    file_size = luaL_checklong(L, arg_i++);
  } else {
    file_size = -1;
  }
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_ftruncate(loop, req, fd, file_size, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.link
 */
static int fs_link(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  const char *new_path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_link(loop, req, path, new_path, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.readdir
 */
static int fs_readdir(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_readdir(loop, req, path, 0, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.readlink
 */
static int fs_readlink(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_readlink(loop, req, path, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.symlink
 */
static int fs_symlink(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  const char *new_path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_symlink(loop, req, path, new_path, 0, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.rename
 */
static int fs_rename(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *old_path = luaL_checkstring(L, arg_i++);
  const char *new_path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_rename(loop, req, old_path, new_path, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.rmdir
 */
static int fs_rmdir(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_rmdir(loop, req, path, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.fstat
 */
static int fs_fstat(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  int fd = luaL_checkint(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_fstat(loop, req, fd, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.lstat
 */
static int fs_lstat(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_lstat(loop, req, path, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.stat
 */
static int fs_stat(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_stat(loop, req, path, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.unlink
 */
static int fs_unlink(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_unlink(loop, req, path, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.utime
 */
static int fs_utime(lua_State* L) {
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
  int arg_n = lua_gettop(L);
  int arg_i = 1;
  const char *path = luaL_checkstring(L, arg_i++);
  lua_Number atime = luaL_checknumber(L, arg_i++);
  lua_Number mtime = luaL_checknumber(L, arg_i++);
  if (arg_i <= arg_n) {
    req->data = fs_checkcallback(L, arg_i++);
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_utime(loop, req, path, atime, mtime, cb);
  return fs_post_handling(L, req);
}

/*
 * fs.open
 */
static int fs_open(lua_State* L) {
  int mode;
  uv_fs_cb cb;
  uv_loop_t *loop = uv_default_loop();
  uv_fs_t *req = alloc_fs_req(L);
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
    cb = on_fs_callback;
  } else {
    req->data = NULL;
    cb = NULL;
  }
  uv_fs_open(loop, req, path, flags, mode, cb);
  return fs_post_handling(L, req);
}

static luaL_reg functions[] = {
  { "chmod",      fs_chmod        }
 ,{ "chown",      fs_chown        }
 ,{ "close",      fs_close        }
 ,{ "exists",     fs_exists       }
 ,{ "fchmod",     fs_fchmod       }
 ,{ "fchown",     fs_fchown       }
 ,{ "fdatasync",  fs_fdatasync    }
 ,{ "fstat",      fs_fstat        }
 ,{ "fsync",      fs_fsync        }
 ,{ "ftruncate",  fs_ftruncate    }
 ,{ "futime",     fs_futime       }
 ,{ "link",       fs_link         }
 ,{ "lstat",      fs_lstat        }
 ,{ "mkdir",      fs_mkdir        }
 ,{ "open",       fs_open         }
 ,{ "read",       fs_read         }
 ,{ "readdir",    fs_readdir      }
 ,{ "readlink",   fs_readlink     }
 ,{ "rename",     fs_rename       }
 ,{ "rmdir",      fs_rmdir        }
 ,{ "stat",       fs_stat         }
 ,{ "symlink",    fs_symlink      }
 ,{ "utime",      fs_utime        }
 ,{ "unlink",     fs_unlink       }
 ,{ "write",      fs_write        }
 ,{ NULL,         NULL            }
};

void luaopen_lev_fs(lua_State *L) {
  /* We put callback function reference identifiers to void * */
  assert(sizeof(void *) >= sizeof(int));

  luaopen_lev_fs_stat(L);

  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  lua_setfield(L, -2, "fs");
}
