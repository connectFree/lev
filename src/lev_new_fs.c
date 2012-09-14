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


typedef struct fs_req_holder_s {
  LEVBASE_REF_FIELDS
  uv_fs_t req;
} fs_req_holder_t;

#define UNWRAP(r) \
  fs_req_holder_t* holder = container_of((r), fs_req_holder_t, req); \
  lua_State* L = (r)->loop->data;

#define FSR__CBNAME "_cb"

#define FSR__SETUP \
  uv_fs_cb cb = NULL;                                                 \
  uv_loop_t *loop = lev_get_loop(L);                                  \
  fs_req_holder_t *holder = (fs_req_holder_t *)create_obj_init_ref(L, \
      sizeof(fs_req_holder_t), "lev.fs");                             \
  /* NOTE: set_call needs "object" to be stack at index 1 */          \
  lua_insert(L, 1);                                                   \
  uv_fs_t *req = &holder->req;

#define FSR__SET_OPT_CB(index, c_callback) \
  if (lua_isfunction(L, (index))) {                               \
    set_callback(L, FSR__CBNAME, (index));                        \
    lev_handle_ref(L, (LevRefStruct_t *)holder, 1);               \
    cb = (c_callback);                                            \
  }

#define FSR__TEARDOWN \
  /* NOTE: remove "object" */                                     \
  lua_remove(L, 1);                                               \
  if (req->result == -1) {                                        \
    lev_push_uv_err(L, LEV_UV_ERR_FROM_REQ(req));                 \
    return 1;                                                     \
  }                                                               \
  if (cb) {                                                       \
    return 0;                                                     \
  } else {                                                        \
    int ret_n = push_results(L, req);                             \
    uv_fs_req_cleanup(req);                                       \
    return ret_n;                                                 \
  }

/*
 * argument utilities
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

static int push_fs_stat(lua_State *L, uv_statbuf_t *s) {
  lua_newtable(L);
  LEV_SET_FIELD(dev, number, s->st_dev);
  LEV_SET_FIELD(ino, number, s->st_ino);
  LEV_SET_FIELD(mode, number, s->st_mode);
  LEV_SET_FIELD(nlink, number, s->st_nlink);
  LEV_SET_FIELD(gid, number, s->st_gid);
  LEV_SET_FIELD(uid, number, s->st_uid);
  LEV_SET_FIELD(rdev, number, s->st_rdev);
  LEV_SET_FIELD(size, number, s->st_size);
  LEV_SET_FIELD(blksize, number, s->st_blksize);
  LEV_SET_FIELD(blocks, number, s->st_blocks);
  LEV_SET_FIELD(atime, number, s->st_atime);
  LEV_SET_FIELD(mtime, number, s->st_mtime);
  LEV_SET_FIELD(ctime, number, s->st_ctime);
  LEV_SET_FIELD(isFile, boolean, S_ISREG(s->st_mode));
  LEV_SET_FIELD(isDirectory, boolean, S_ISDIR(s->st_mode));
  LEV_SET_FIELD(isCharacterDevice, boolean, S_ISCHR(s->st_mode));
  LEV_SET_FIELD(isBlockDevice, boolean, S_ISBLK(s->st_mode));
  LEV_SET_FIELD(isFIFO, boolean, S_ISFIFO(s->st_mode));
  LEV_SET_FIELD(isSymbolicLink, boolean, S_ISLNK(s->st_mode));
  LEV_SET_FIELD(isSocket, boolean, S_ISSOCK(s->st_mode));
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

/*
 * pushing results
 *
 * NOTE: write returns err, written_byte_count, which is different from
 *       Node.js (err, written_byte_count, buffer)
 */
static int push_results(lua_State *L, uv_fs_t *req) {
  if (req->result == -1) {
    lev_push_uv_err(L, LEV_UV_ERR_FROM_REQ(req));
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
  UNWRAP(req);
  int ret_n;

  push_callback_no_obj(L, holder, FSR__CBNAME);
  ret_n = push_results(L, req);
  lua_call(L, ret_n, 0);

  uv_fs_req_cleanup(req);
  lev_handle_unref(L, (LevRefStruct_t *)holder);
}

/*
 * fs.exists
 */

static int push_exists_results(lua_State *L, uv_fs_t *req) {
  lua_pushboolean(L, req->result != -1);
  return 1;
}

static void on_exists_callback(uv_fs_t *req) {
  UNWRAP(req);
  int ret_n;

  push_callback_no_obj(L, holder, FSR__CBNAME);
  ret_n = push_exists_results(L, req);
  lua_call(L, ret_n, 0);

  uv_fs_req_cleanup(req);
  lev_handle_unref(L, (LevRefStruct_t *)holder);
}

static int fs_exists(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  FSR__SET_OPT_CB(3, on_exists_callback)
  uv_fs_stat(loop, req, path, cb);
  lua_pushboolean(L, req->result != -1);
  if (!cb) {
    uv_fs_req_cleanup(req);
  }
  /* NOTE: remove "object" */
  lua_remove(L, 1);
  return 1;
}

/*
 * fs.close
 */
static int fs_close(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  int fd = luaL_checkint(L, 2);
  FSR__SET_OPT_CB(3, on_fs_callback)
  uv_fs_close(loop, req, fd, cb);
  FSR__TEARDOWN
}

/*
 * fs.chmod
 */
static int fs_chmod(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  int mode = luaL_checkint(L, 3);
  FSR__SET_OPT_CB(4, on_fs_callback)
  uv_fs_chmod(loop, req, path, mode, cb);
  FSR__TEARDOWN
}

/*
 * fs.chown
 */
static int fs_chown(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  int uid = luaL_checkint(L, 3);
  int gid = luaL_checkint(L, 4);
  FSR__SET_OPT_CB(5, on_fs_callback)
  uv_fs_chown(loop, req, path, uid, gid, cb);
  FSR__TEARDOWN
}

/*
 * fs.fchmod
 */
static int fs_fchmod(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  int fd = luaL_checkint(L, 2);
  int mode = luaL_checkint(L, 3);
  FSR__SET_OPT_CB(4, on_fs_callback)
  uv_fs_fchmod(loop, req, fd, mode, cb);
  FSR__TEARDOWN
}

/*
 * fs.fchown
 */
static int fs_fchown(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  int fd = luaL_checkint(L, 2);
  int uid = luaL_checkint(L, 3);
  int gid = luaL_checkint(L, 4);
  FSR__SET_OPT_CB(5, on_fs_callback)
  uv_fs_fchown(loop, req, fd, uid, gid, cb);
  FSR__TEARDOWN
}

/*
 * fs.fdatasync
 */
static int fs_fdatasync(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  int fd = luaL_checkint(L, 2);
  FSR__SET_OPT_CB(3, on_fs_callback)
  uv_fs_fdatasync(loop, req, fd, cb);
  FSR__TEARDOWN
}

/*
 * fs.fsync
 */
static int fs_fsync(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  int fd = luaL_checkint(L, 2);
  FSR__SET_OPT_CB(3, on_fs_callback)
  uv_fs_fsync(loop, req, fd, cb);
  FSR__TEARDOWN
}

/*
 * fs.futime
 */
static int fs_futime(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  int fd = luaL_checkint(L, 2);
  lua_Number atime = luaL_checknumber(L, 3);
  lua_Number mtime = luaL_checknumber(L, 4);
  FSR__SET_OPT_CB(5, on_fs_callback)
  uv_fs_futime(loop, req, fd, atime, mtime, cb);
  FSR__TEARDOWN
}

/*
 * fs.mkdir
 */
static int fs_mkdir(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  int mode = luaL_optint(L, 3, 0777);
  FSR__SET_OPT_CB(4, on_fs_callback)
  uv_fs_mkdir(loop, req, path, mode, cb);
  FSR__TEARDOWN
}

/*
 * fs.read
 */
static int fs_read(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  int fd = luaL_checkint(L, 2);
  MemSlice *ms = lev_checkbuffer(L, 3);
  int64_t file_pos = luaL_optlong(L, 4, 0);
  file_pos--; /* Luaisms */
  size_t file_until = luaL_optlong(L, 5, 0);
  if (file_until > ms->until) {
    file_until = ms->until;
  }
  FSR__SET_OPT_CB(5, on_fs_callback)
  uv_fs_read(loop, req, fd, ms->slice, (file_until ? file_until : ms->until), file_pos, cb);
  FSR__TEARDOWN
}

/*
 * fs.write
 */
static int fs_write(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  int fd = luaL_checkint(L, 2);
  MemSlice *ms = lev_checkbuffer(L, 3);
  int64_t file_pos = luaL_optlong(L, 4, 0);
  file_pos--; /* Luaisms */
  size_t file_until = luaL_optlong(L, 5, 0);
  if (file_until > ms->until) {
    file_until = ms->until;
  }
  FSR__SET_OPT_CB(5, on_fs_callback)
  uv_fs_write(loop, req, fd, ms->slice, (file_until ? file_until : ms->until), file_pos, cb);
  FSR__TEARDOWN
}

/*
 * fs.ftruncate
 */
static int fs_ftruncate(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  int fd = luaL_checkint(L, 2);
  int64_t file_size = luaL_checklong(L, 3);
  FSR__SET_OPT_CB(4, on_fs_callback)
  uv_fs_ftruncate(loop, req, fd, file_size, cb);
  FSR__TEARDOWN
}

/*
 * fs.link
 */
static int fs_link(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  const char *new_path = luaL_checkstring(L, 3);
  FSR__SET_OPT_CB(4, on_fs_callback)
  uv_fs_link(loop, req, path, new_path, cb);
  FSR__TEARDOWN
}

/*
 * fs.readdir
 */
static int fs_readdir(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  FSR__SET_OPT_CB(3, on_fs_callback)
  uv_fs_readdir(loop, req, path, 0, cb);
  FSR__TEARDOWN
}

/*
 * fs.readlink
 */
static int fs_readlink(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  FSR__SET_OPT_CB(3, on_fs_callback)
  uv_fs_readlink(loop, req, path, cb);
  FSR__TEARDOWN
}

/*
 * fs.symlink
 */
static int fs_symlink(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  const char *new_path = luaL_checkstring(L, 3);
  FSR__SET_OPT_CB(4, on_fs_callback)
  uv_fs_symlink(loop, req, path, new_path, 0, cb);
  FSR__TEARDOWN
}

/*
 * fs.rename
 */
static int fs_rename(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *old_path = luaL_checkstring(L, 2);
  const char *new_path = luaL_checkstring(L, 3);
  FSR__SET_OPT_CB(4, on_fs_callback)
  uv_fs_rename(loop, req, old_path, new_path, cb);
  FSR__TEARDOWN
}

/*
 * fs.rmdir
 */
static int fs_rmdir(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  FSR__SET_OPT_CB(3, on_fs_callback)
  uv_fs_rmdir(loop, req, path, cb);
  FSR__TEARDOWN
}

/*
 * fs.fstat
 */
static int fs_fstat(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  int fd = luaL_checkint(L, 2);
  FSR__SET_OPT_CB(3, on_fs_callback)
  uv_fs_fstat(loop, req, fd, cb);
  FSR__TEARDOWN
}

/*
 * fs.lstat
 */
static int fs_lstat(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  FSR__SET_OPT_CB(3, on_fs_callback)
  uv_fs_lstat(loop, req, path, cb);
  FSR__TEARDOWN
}

/*
 * fs.stat
 */
static int fs_stat(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  FSR__SET_OPT_CB(3, on_fs_callback)
  uv_fs_stat(loop, req, path, cb);
  FSR__TEARDOWN
}

/*
 * fs.unlink
 */
static int fs_unlink(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  FSR__SET_OPT_CB(3, on_fs_callback)
  uv_fs_unlink(loop, req, path, cb);
  FSR__TEARDOWN
}

/*
 * fs.utime
 */
static int fs_utime(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  lua_Number atime = luaL_checknumber(L, 3);
  lua_Number mtime = luaL_checknumber(L, 4);
  FSR__SET_OPT_CB(5, on_fs_callback)
  uv_fs_utime(loop, req, path, atime, mtime, cb);
  FSR__TEARDOWN
}


/*
 * fs.open
 */
static int fs_open(lua_State* L) {
  FSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *path = luaL_checkstring(L, 2);
  int flags = fs_checkflags(L, 3);
  int mode = luaL_optint(L, 4, 0666);
  FSR__SET_OPT_CB(5, on_fs_callback)
  uv_fs_open(loop, req, path, flags, mode, cb);
  FSR__TEARDOWN
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
  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  lua_setfield(L, -2, "fs");
}
