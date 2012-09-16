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
#include "time_cache.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <grp.h>
#include <errno.h>
#include <sys/utsname.h>

#include <lua.h>
#include <lauxlib.h>

#ifndef PATH_MAX
#define PATH_MAX (8096)
#endif

#ifndef MAX_TITLE_LENGTH
#define MAX_TITLE_LENGTH (8192)
#endif

#ifndef _WIN32

const char *core_signo_string(int signo) {
#define SIGNO_CASE(e)  case e: return #e;
  switch (signo) {

#ifdef SIGHUP
  SIGNO_CASE(SIGHUP);
#endif

#ifdef SIGINT
  SIGNO_CASE(SIGINT);
#endif

#ifdef SIGQUIT
  SIGNO_CASE(SIGQUIT);
#endif

#ifdef SIGILL
  SIGNO_CASE(SIGILL);
#endif

#ifdef SIGTRAP
  SIGNO_CASE(SIGTRAP);
#endif

#ifdef SIGABRT
  SIGNO_CASE(SIGABRT);
#endif

#ifdef SIGIOT
# if SIGABRT != SIGIOT
  SIGNO_CASE(SIGIOT);
# endif
#endif

#ifdef SIGBUS
  SIGNO_CASE(SIGBUS);
#endif

#ifdef SIGFPE
  SIGNO_CASE(SIGFPE);
#endif

#ifdef SIGKILL
  SIGNO_CASE(SIGKILL);
#endif

#ifdef SIGUSR1
  SIGNO_CASE(SIGUSR1);
#endif

#ifdef SIGSEGV
  SIGNO_CASE(SIGSEGV);
#endif

#ifdef SIGUSR2
  SIGNO_CASE(SIGUSR2);
#endif

#ifdef SIGPIPE
  SIGNO_CASE(SIGPIPE);
#endif

#ifdef SIGALRM
  SIGNO_CASE(SIGALRM);
#endif

#ifdef SIGTERM
  SIGNO_CASE(SIGTERM);
#endif

#ifdef SIGCHLD
  SIGNO_CASE(SIGCHLD);
#endif

#ifdef SIGSTKFLT
  SIGNO_CASE(SIGSTKFLT);
#endif


#ifdef SIGCONT
  SIGNO_CASE(SIGCONT);
#endif

#ifdef SIGSTOP
  SIGNO_CASE(SIGSTOP);
#endif

#ifdef SIGTSTP
  SIGNO_CASE(SIGTSTP);
#endif

#ifdef SIGTTIN
  SIGNO_CASE(SIGTTIN);
#endif

#ifdef SIGTTOU
  SIGNO_CASE(SIGTTOU);
#endif

#ifdef SIGURG
  SIGNO_CASE(SIGURG);
#endif

#ifdef SIGXCPU
  SIGNO_CASE(SIGXCPU);
#endif

#ifdef SIGXFSZ
  SIGNO_CASE(SIGXFSZ);
#endif

#ifdef SIGVTALRM
  SIGNO_CASE(SIGVTALRM);
#endif

#ifdef SIGPROF
  SIGNO_CASE(SIGPROF);
#endif

#ifdef SIGWINCH
  SIGNO_CASE(SIGWINCH);
#endif

#ifdef SIGIO
  SIGNO_CASE(SIGIO);
#endif

#ifdef SIGPOLL
# if SIGPOLL != SIGIO
  SIGNO_CASE(SIGPOLL);
# endif
#endif

#ifdef SIGLOST
  SIGNO_CASE(SIGLOST);
#endif

#ifdef SIGPWR
# if SIGPWR != SIGLOST
  SIGNO_CASE(SIGPWR);
# endif
#endif

#ifdef SIGSYS
  SIGNO_CASE(SIGSYS);
#endif

  }
  return "";
}


static void core_on_signal(struct ev_loop *loop, struct ev_signal *w, int revents) {
  assert(uv_default_loop()->ev == loop);
  lua_State* L = (lua_State*)w->data;
  lua_getglobal(L, "process");
  lua_getfield(L, -1, "call");
  lua_pushvalue(L, -2);
  lua_remove(L, -3);
  lua_pushstring(L, core_signo_string(w->signum));
  lua_pushinteger(L, revents);
  lua_call(L, 3, 0);
}

#endif

static int core_activate_signal_handler(lua_State* L) {
#ifndef _WIN32
  int signal = luaL_checkint(L, 1);
  struct ev_signal* signal_watcher = (struct ev_signal*)malloc(sizeof(struct ev_signal));
  signal_watcher->data = L;
  ev_signal_init (signal_watcher, core_on_signal, signal);
  struct ev_loop* loop = lev_get_loop(L)->ev;
  ev_signal_start (loop, signal_watcher);
#endif
  return 0;
}

static int core_update_time(lua_State* L) {
  uv_update_time(lev_get_loop(L));
  return 0;
}

static int core_now(lua_State* L) {
  double now = (double)uv_now(lev_get_loop(L));
  lua_pushnumber(L, now);
  return 1;
}

static int core_hrtime(lua_State* L) {
  double now = (double) uv_hrtime() / 1000000.0;
  lua_pushnumber(L, now);
  return 1;
}

static int core_get_free_memory(lua_State* L) {
  lua_pushnumber(L, uv_get_free_memory());
  return 1;
}

static int core_get_total_memory(lua_State* L) {
  lua_pushnumber(L, uv_get_total_memory());
  return 1;
}

static int core_loadavg(lua_State* L) {
  double avg[3];
  uv_loadavg(avg);
  lua_pushnumber(L, avg[0]);
  lua_pushnumber(L, avg[1]);
  lua_pushnumber(L, avg[2]);
  return 3;
}

static int core_uptime(lua_State* L) {
  double uptime;
  uv_uptime(&uptime);
  lua_pushnumber(L, uptime);
  return 1;
}

static int core_cpu_info(lua_State* L) {
  uv_cpu_info_t* cpu_infos;
  int count, i;
  uv_cpu_info(&cpu_infos, &count);
  lua_newtable(L);

  for (i = 0; i < count; i++) {
    lua_newtable(L);
    lua_pushstring(L, (cpu_infos[i]).model);
    lua_setfield(L, -2, "model");
    lua_pushnumber(L, (cpu_infos[i]).speed);
    lua_setfield(L, -2, "speed");
    lua_newtable(L);
    lua_pushnumber(L, (cpu_infos[i]).cpu_times.user);
    lua_setfield(L, -2, "user");
    lua_pushnumber(L, (cpu_infos[i]).cpu_times.nice);
    lua_setfield(L, -2, "nice");
    lua_pushnumber(L, (cpu_infos[i]).cpu_times.sys);
    lua_setfield(L, -2, "sys");
    lua_pushnumber(L, (cpu_infos[i]).cpu_times.idle);
    lua_setfield(L, -2, "idle");
    lua_pushnumber(L, (cpu_infos[i]).cpu_times.irq);
    lua_setfield(L, -2, "irq");
    lua_setfield(L, -2, "times");
    lua_rawseti(L, -2, i + 1);
  }

  uv_free_cpu_info(cpu_infos, count);
  return 1;
}

#if 0
 struct uv_interface_address_s {
   char* name;
   int is_internal;
   union {
     struct sockaddr_in address4;
     struct sockaddr_in6 address6;
   } address;
 };
#endif

static int core_interface_addresses(lua_State* L) {
  uv_interface_address_t* interfaces;
  int count, i;
  char ip[INET6_ADDRSTRLEN];

  uv_interface_addresses(&interfaces, &count);

  lua_newtable(L);

  for (i = 0; i < count; i++) {
    const char* family;

    lua_getfield(L, -1, interfaces[i].name);
    if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      lua_newtable(L);
      lua_pushvalue(L, -1);
      lua_setfield(L, -3, interfaces[i].name);
    }
    lua_newtable(L);
    lua_pushboolean(L, interfaces[i].is_internal);
    lua_setfield(L, -2, "internal");

    if (interfaces[i].address.address4.sin_family == AF_INET) {
      uv_ip4_name(&interfaces[i].address.address4,ip, sizeof(ip));
      family = "IPv4";
    } else if (interfaces[i].address.address4.sin_family == AF_INET6) {
      uv_ip6_name(&interfaces[i].address.address6, ip, sizeof(ip));
      family = "IPv6";
    } else {
      strncpy(ip, "<unknown sa family>", INET6_ADDRSTRLEN);
      family = "<unknown>";
    }
    lua_pushstring(L, ip);
    lua_setfield(L, -2, "address");
    lua_pushstring(L, family);
    lua_setfield(L, -2, "family");
    lua_rawseti(L, -2, lua_objlen (L, -2) + 1);
    lua_pop(L, 1);
  }
  uv_free_interface_addresses(interfaces, count);
  return 1;
}

static int core_execpath(lua_State* L) {
  size_t size = 2*PATH_MAX;
  char exec_path[2*PATH_MAX];
  if (uv_exepath(exec_path, &size)) {
    uv_err_t err = uv_last_error(lev_get_loop(L));
    return luaL_error(L, uv_strerror(err));
  }
  lua_pushlstring(L, exec_path, size);
  return 1;
}

int core_handle_type(lua_State *L) {
  uv_file file = luaL_checkint(L, 1);
  uv_handle_type type = uv_guess_handle(file);
  lua_pushstring(L, luv_handle_type_to_string(type));
  return 1;
}

static int core_timecache_http(lua_State* L) {
  cache_time_update();
  lua_pushstring(L, (char *)http_time);
  return 1;
}

static int core_timecache_httplog(lua_State* L) {
  cache_time_update();
  lua_pushstring(L, (char *)http_log_time);
  return 1;
}

static int core_timecache_errorlog(lua_State* L) {
  cache_time_update();
  lua_pushstring(L, (char *)err_log_time);
  return 1;
}

static int core_getuid(lua_State *L) {
  lua_pushinteger(L, getpid());
  return 1;
}

static int core_getgid(lua_State *L) {
  lua_pushinteger(L, getgid());
  return 1;
}

static int core_setuid(lua_State *L) {
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
      return luaL_error(L, "cannot get password entry buffer size");
    }

    mb = lev_slab_getBlock(bufsize);
    lev_slab_incRef(mb);
    err = getpwnam_r(name, &pwd, (char *)mb->bytes, bufsize, &pwdp);
    if (err || pwdp == NULL) {
      return luaL_error(L, "user id \"%s\" does not exist", name);
    }
    uid = pwdp->pw_uid;
    lev_slab_decRef(mb);
  } else {
    return luaL_error(L, "number or string expected");
  }

  err = setuid(uid);
  if (err) {
    return luaL_error(L, "setuid error (%d)", errno);
  }

  return 0;
}

static int core_setgid(lua_State *L) {
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
      return luaL_error(L, "cannot get password entry buffer size");
    }

    mb = lev_slab_getBlock(bufsize);
    lev_slab_incRef(mb);
    err = getgrnam_r(name, &grp, (char *)mb->bytes, bufsize, &grpp);
    if (err || grpp == NULL) {
      return luaL_error(L, "group id \"%s\" does not exist", name);
    }
    gid = grpp->gr_gid;
    lev_slab_decRef(mb);
  } else {
    return luaL_error(L, "number or string expected");
  }

  err = setgid(gid);
  if (err) {
    return luaL_error(L, "setgid error (%d)", errno);
  }

  return 0;
}

static int core_umask(lua_State *L) {
  int argc = lua_gettop(L);
  unsigned int old;

  if (argc < 1) {
    old = umask(0);
    umask((int)old);
  } else {
    int type = lua_type(L, -1);
    if (type != LUA_TNUMBER && type != LUA_TSTRING) {
      return luaL_error(L, "number or string expected");
    } else if (type == LUA_TSTRING && !lua_isnumber(L, -1)) {
      return luaL_error(L, "invalid string value");
    }
    old = lua_tonumber(L, -1);
    old = umask((int)old);
  }

  lua_pushnumber(L, old);
  return 1;
}

/* TODO: should be support debug modeule
extern void uv_print_active_handles(uv_loop_t *loop);
extern void uv_print_all_handles(uv_loop_t *loop);

int core_print_active_handles(lua_State* L) {
  uv_print_active_handles(lev_get_loop(L));
  return 0;
}

int core_print_all_handles(lua_State* L) {
  uv_print_all_handles(lev_get_loop(L));
  return 0;
}
*/


static luaL_reg functions[] = {
   {"activate_signal_handler", core_activate_signal_handler}
  ,{"update_time", core_update_time}
  ,{"now", core_now}
  ,{"hrtime", core_hrtime}
  ,{"get_free_memory", core_get_free_memory}
  ,{"get_total_memory", core_get_total_memory}
  ,{"loadavg", core_loadavg}
  ,{"uptime", core_uptime}
  ,{"cpu_info", core_cpu_info}
  ,{"interface_addresses", core_interface_addresses}
  ,{"execpath", core_execpath}
  ,{"handle_type", core_handle_type}
  ,{"timeHTTP", core_timecache_http}
  ,{"timeHTTPLog", core_timecache_httplog}
  ,{"timeELog", core_timecache_errorlog}
  ,{"getuid", core_getuid}
  ,{"getgid", core_getgid}
  ,{"setuid", core_setuid}
  ,{"setgid", core_setgid}
  ,{"umask", core_umask}
  /* TODO: should be support debug module.
  ,{"print_active_handles", core_print_active_handles}
  ,{"print_all_handles", core_print_all_handles}
  */
  ,{ NULL, NULL }
};


static int core_platform(lua_State *L) {
#ifdef WIN32
  lua_pushstring(L, "win32");
#else
  struct utsname info;
  char *p;

  uname(&info);
  for (p = info.sysname; *p; p++) {
    *p = (char)tolower((unsigned char)*p);
  }
  lua_pushstring(L, info.sysname);
#endif
  return 1;
}

static int core_arch(lua_State *L) {
  struct utsname info;
  uname(&info);
  lua_pushstring(L, info.machine);
  return 1;
}

static int core_version(lua_State *L) {
  lua_pushstring(L, LEV_VERSION);
  return 1;
}

typedef struct core_version_s {
  char *module;
  char *version;
} core_version_t;

static core_version_t versions[] = {
   { "lev", LEV_VERSION }
  ,{ "luajit", LUAJIT_VERSION }
  ,{ "http_perser", HTTP_VERSION }
  ,{ "libuv", UV_VERSION }
  ,{ NULL, NULL } /* sentinail */
};

static int core_versions(lua_State *L) {
  lua_newtable(L);
  core_version_t *ver;
  for (ver = versions; ver->module; ver++) {
    lua_pushstring(L, ver->version);
    lua_setfield(L, -2, ver->module);
  }
  return 1;
}


void luaopen_lev_core(lua_State *L) {
  cache_time_init();
  luaL_register(L, NULL, functions);

  /* set properties */
  core_platform(L);
  lua_setfield(L, -2, "platform");
  core_arch(L);
  lua_setfield(L, -2, "arch");
  core_version(L);
  lua_setfield(L, -2, "version");
  core_versions(L);
  lua_setfield(L, -2, "versions");
}
