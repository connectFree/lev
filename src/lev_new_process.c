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

#ifndef WIN32
#include <ctype.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>
#endif

/*

  X:TODO Beginning of PROCESS Module...
  
*/

#define UNWRAP(h) \
  process_obj* self = container_of((h), process_obj, handle); \
  lua_State* L = self->handle.loop->data;

typedef struct {
  LEVBASE_REF_FIELDS
} process_obj;

static int process_new(lua_State* L) {
  uv_loop_t* loop;
  process_obj* self;

  loop = lev_get_loop(L);
  assert(L == loop->data);

  self = (process_obj*)create_obj_init_ref(L, sizeof *self, "lev.process");

  return 1;
}

static int process_cwd(lua_State* L) {
  size_t size = 2*PATH_MAX-1;
  char path[2*PATH_MAX];
  uv_err_t err;

  err = uv_cwd(path, size);
  if (err.code != UV_OK) {
    return luaL_error(L, "uv_cwd: %s", uv_strerror(err));
  }
  lua_pushstring(L, path);
  return 1;
}

#ifndef WIN32
static int process_pid(lua_State* L) {
  int pid = getpid();
  lua_pushinteger(L, pid);
  return 1;
}

static int process_setgid(lua_State* L) {
  int gid;
  int err;
  int type = lua_type(L, -1);
  if (type == LUA_TNUMBER) {
    gid = lua_tonumber(L, -1);
  } else if (type == LUA_TSTRING) {
    MemBlock *mb;
    struct group grp;
    struct group *grpp = NULL;
    const char *name = lua_tostring(L, -1);
    int bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufsize == -1) {
      return luaL_error(L, "process.setgid: cannot get password entry buffer size");
    }

    mb = lev_slab_getBlock(bufsize);
    lev_slab_incRef(mb);
    err = getgrnam_r(name, &grp, (char *)mb->bytes, bufsize, &grpp);
    if (err || grpp == NULL) {
      return luaL_error(L, "process.setgid: group id \"%s\" does not exist", name);
    }
    gid = grpp->gr_gid;
    lev_slab_decRef(mb);
  } else {
    return luaL_error(L, "process.setgid: number or string expected");
  }

  err = setgid(gid);
  if (err) {
    return luaL_error(L, "process.setgid: errno=%d", errno);
  }

  return 0;
}

static int process_getgid(lua_State* L) {
  int gid = getgid();
  lua_pushinteger(L, gid);
  return 1;
}

static int process_setuid(lua_State* L) {
  int uid;
  int err;
  int type = lua_type(L, -1);
  if (type == LUA_TNUMBER) {
    uid = lua_tonumber(L, -1);
  } else if (type == LUA_TSTRING) {
    MemBlock *mb;
    struct passwd pwd;
    struct passwd *pwdp = NULL;
    const char *name = lua_tostring(L, -1);
    int bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufsize == -1) {
      return luaL_error(L, "process.setuid: cannot get password entry buffer size");
    }

    mb = lev_slab_getBlock(bufsize);
    lev_slab_incRef(mb);
    err = getpwnam_r(name, &pwd, (char *)mb->bytes, bufsize, &pwdp);
    if (err || pwdp == NULL) {
      return luaL_error(L, "process.setuid: user id \"%s\" does not exist", name);
    }
    uid = pwdp->pw_uid;
    lev_slab_decRef(mb);
  } else {
    return luaL_error(L, "process.setuid: number or string expected");
  }

  err = setuid(uid);
  if (err) {
    return luaL_error(L, "process.setuid: errno=%d", errno);
  }

  return 0;
}

static int process_getuid(lua_State* L) {
  int uid = getuid();
  lua_pushinteger(L, uid);
  return 1;
}
#endif

#define LEV_SETENV_ERRNO_MAP(XX) \
  XX(EINVAL) \
  XX(ENOMEM)
LEV_STD_ERRNAME_FUNC(lev_setenv_errname, LEV_SETENV_ERRNO_MAP, EUNDEF)

static int process_getenv(lua_State* L) {
  const char *name = luaL_checkstring(L, 1);
  char *value = getenv(name);
  if (value) {
    lua_pushstring(L, value);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int process_setenv(lua_State* L) {
  const char *name = luaL_checkstring(L, 1);
  const char *value = luaL_checkstring(L, 2);
  int r = setenv(name, value, 1);
  if (r) {
    lua_pushstring(L, lev_setenv_errname(errno));
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int process_unsetenv(lua_State* L) {
  const char *name = luaL_checkstring(L, 1);
  int r = unsetenv(name);
  if (r) {
    lua_pushstring(L, lev_setenv_errname(errno));
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static luaL_reg methods[] = {
   /*{ "method_name",     ...      }*/
  { NULL,         NULL            }
};


static luaL_reg functions[] = {
   { "new", process_new }
  ,{ "cwd", process_cwd }
  ,{ "getenv", process_getenv }
  ,{ "setenv", process_setenv }
  ,{ "unsetenv", process_unsetenv }
  ,{ "pid", process_pid }
#ifndef WIN32
  ,{ "setgid", process_setgid }
  ,{ "getgid", process_getgid }
  ,{ "setuid", process_setuid }
  ,{ "getuid", process_getuid }
#endif
  ,{ NULL, NULL }
};


#define PROPERTY_COUNT 1

static int process_platform(lua_State* L) {
#ifdef WIN32
  lua_pushstring(L, "win32");
#else
  struct utsname info;
  char *p;

  uname(&info);
  for (p = info.sysname; *p; p++)
    *p = (char)tolower((unsigned char)*p);
  lua_pushstring(L, info.sysname);
#endif
  return 1;
}

void luaopen_lev_process(lua_State *L) {
  luaL_newmetatable(L, "lev.process");
  luaL_register(L, NULL, methods);
  lua_setfield(L, -1, "__index");

  lua_createtable(L, 0, ARRAY_SIZE(functions) + PROPERTY_COUNT - 1);
  luaL_register(L, NULL, functions);

  /* set properties */
  process_platform(L);
  lua_setfield(L, -2, "platform");

  lua_setfield(L, -2, "process");
}
