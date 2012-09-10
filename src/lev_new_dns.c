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

  X:TODO Beginning of DNS Module...
  
*/

/*
 * getaddrinfo request memory management
 */
#define MANAGE_REQ_IN_LUA

#ifdef MANAGE_REQ_IN_LUA
typedef struct getaddrinfo_req_holder_s {
  int ref;
  int callback_ref;
  int getnameinfo_flags;
  uv_getaddrinfo_t req;
} getaddrinfo_req_holder_t;
#endif

static uv_getaddrinfo_t *alloc_getaddrinfo_req(lua_State *L) {
#ifdef MANAGE_REQ_IN_LUA
  getaddrinfo_req_holder_t *holder =
    lua_newuserdata(L, sizeof(getaddrinfo_req_holder_t));
  /* To avoid being gc'ed, we put this to registry. */
  holder->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  return &holder->req;
#else
  MemBlock *block = lev_slab_getBlock(sizeof(uv_getaddrinfo_t));
  lev_slab_incRef(block);
  return block->bytes;
#endif
}

static dispose_getaddrinfo_req(uv_getaddrinfo_t *req) {
#ifdef MANAGE_REQ_IN_LUA
  getaddrinfo_req_holder_t *holder =
    container_of(req, getaddrinfo_req_holder_t, req);
  lua_State *L = req->loop->data;
  luaL_unref(L, LUA_REGISTRYINDEX, holder->ref);
#else
  MemBlock* block = container_of(req, MemBlock, bytes);
  lev_slab_decRef(block);
#endif
}

/*
 * callback utilities
 *
 * TODO: remove duplicated code once udp module is merged.
 */

static int getaddrinfo_checkcallback(lua_State *L, int index) {
  luaL_checktype(L, index, LUA_TFUNCTION);
  lua_pushvalue(L, index);
  return luaL_ref(L, LUA_REGISTRYINDEX);
}

static void getaddrinfo_push_callback(lua_State *L, int ref) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, ref);

  /*
   * We can safely call luaL_unref() here because now the stack holds the value
   * so it is not garbage collected.
   */
  luaL_unref(L, LUA_REGISTRYINDEX, ref);
}

static void lev_push_uv_err(lua_State *L, uv_err_t err) {
  lua_createtable(L, 0, 2);

  lua_pushstring(L, uv_strerror(err));
  lua_setfield(L, -2, "message");

  lua_pushnumber(L, err.code);
  lua_setfield(L, -2, "code");
}

static uv_err_t lev_code_to_uv_err(uv_err_code errcode) {
  uv_err_t err;
  err.code = errcode;
  return err;
}

static void on_getaddrinfo_callback(uv_getaddrinfo_t *req, int status,
    struct addrinfo *res) {
  uv_loop_t *loop = req->loop;
  lua_State *L = loop->data;
  int arg_n;

  getaddrinfo_req_holder_t *holder =
    container_of(req, getaddrinfo_req_holder_t, req);
  getaddrinfo_push_callback(L, holder->callback_ref);
  if (status == -1) {
    arg_n = 1;
    lev_push_uv_err(L, uv_last_error(loop));
  } else {
    arg_n = 2;
    lua_pushnil(L);

    lua_newtable(L);
    char host_buf[NI_MAXHOST];
    char serv_buf[NI_MAXHOST];
    struct addrinfo *ai;
    int i;
    for (ai = res, i = 1; ai; ai = ai->ai_next, ++i) {
      int r = getnameinfo(ai->ai_addr, ai->ai_addrlen,
          host_buf, sizeof(host_buf),
          serv_buf, sizeof(serv_buf),
          holder->getnameinfo_flags);
      if (r) {
        lua_pop(L, 2);

        arg_n = 1;

        lua_newtable(L);

        lua_pushnumber(L, r);
        lua_setfield(L, -2, "code");

        lua_pushstring(L, gai_strerror(r));
        lua_setfield(L, -2, "message");
      }

      lua_newtable(L);

      lua_pushnumber(L, ai->ai_flags);
      lua_setfield(L, -2, "flags");

      lua_pushnumber(L, ai->ai_family);
      lua_setfield(L, -2, "family");

      lua_pushnumber(L, ai->ai_socktype);
      lua_setfield(L, -2, "socktype");

      lua_pushnumber(L, ai->ai_protocol);
      lua_setfield(L, -2, "protocol");

      lua_pushstring(L, ai->ai_canonname);
      lua_setfield(L, -2, "canonname");

      lua_pushstring(L, host_buf);
      lua_setfield(L, -2, "host");

      lua_pushstring(L, serv_buf);
      lua_setfield(L, -2, "service");

      lua_rawseti(L, -2, i);
    }
  }
  lua_call(L, arg_n, 0);
  dispose_getaddrinfo_req(req);
  uv_freeaddrinfo(res);
}

static int dns_getaddrinfo(lua_State* L) {
  uv_getaddrinfo_cb cb = on_getaddrinfo_callback;
  uv_loop_t *loop = luv_get_loop(L);
  uv_getaddrinfo_t *req = alloc_getaddrinfo_req(L);
  getaddrinfo_req_holder_t *holder =
    container_of(req, getaddrinfo_req_holder_t, req);
  const char *host = luaL_optstring(L, 1, NULL);
  const char *serv = luaL_optstring(L, 2, NULL);

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = PF_UNSPEC;
  if (lua_istable(L, 3)) {
#define SET_ADDRINFO_FIELD(name) \
  lua_getfield(L, 3, #name); \
  if (lua_isnumber(L, -1)) { \
    hints.ai_##name = lua_tonumber(L, -1); \
  } else if (!lua_isnil(L, -1)) { \
    return luaL_argerror(L, 3, "field '##name' must be of type 'number'"); \
  } \
  lua_pop(L, 1);

    SET_ADDRINFO_FIELD(family);
    SET_ADDRINFO_FIELD(socktype);
    SET_ADDRINFO_FIELD(protocol);
    SET_ADDRINFO_FIELD(flags);
  } else if (!lua_isnil(L, 3)) {
    return luaL_argerror(L, 3, "Must be of type 'table' or nil");
  }

  holder->getnameinfo_flags = luaL_optint(L, 4, 0);
  holder->callback_ref = getaddrinfo_checkcallback(L, 5);

  int r = uv_getaddrinfo(loop, req, cb, host, serv, &hints);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(loop));
    return 1;
  }

  lua_pushnil(L);
  return 1;
}


static luaL_reg functions[] = {
   { "getaddrinfo", dns_getaddrinfo }
  ,{ NULL, NULL }
};


static void define_constants(lua_State *L) {
#define DEFINE_CONSTANT(name) \
  lua_pushnumber(L, name); \
  lua_setfield(L, -2, #name);

  /* ai_flags values */
  DEFINE_CONSTANT(AI_PASSIVE);
  DEFINE_CONSTANT(AI_CANONNAME);
  DEFINE_CONSTANT(AI_NUMERICHOST);

  /* ai_family values */
  DEFINE_CONSTANT(PF_UNSPEC);
  DEFINE_CONSTANT(PF_INET);
  DEFINE_CONSTANT(PF_INET6);

  /* ai_socktype values */
  DEFINE_CONSTANT(SOCK_STREAM);
  DEFINE_CONSTANT(SOCK_DGRAM);
  DEFINE_CONSTANT(SOCK_RAW);

  /* ai_protocol values */
  DEFINE_CONSTANT(IPPROTO_UDP);
  DEFINE_CONSTANT(IPPROTO_TCP);

  /* getnameinfo flags values */
  DEFINE_CONSTANT(NI_NOFQDN);
  DEFINE_CONSTANT(NI_NUMERICHOST);
  DEFINE_CONSTANT(NI_NAMEREQD);
  DEFINE_CONSTANT(NI_NUMERICSERV);
  DEFINE_CONSTANT(NI_DGRAM);

  /* getaddrinfo error codes */
#ifdef EAI_ADDRFAMILY
  DEFINE_CONSTANT(EAI_ADDRFAMILY);
#endif
  DEFINE_CONSTANT(EAI_AGAIN);
  DEFINE_CONSTANT(EAI_BADFLAGS);
  DEFINE_CONSTANT(EAI_FAIL);
  DEFINE_CONSTANT(EAI_FAMILY);
  DEFINE_CONSTANT(EAI_MEMORY);
#ifdef EAI_NODATA
  DEFINE_CONSTANT(EAI_NODATA);
#endif
  DEFINE_CONSTANT(EAI_NONAME);
  DEFINE_CONSTANT(EAI_SERVICE);
  DEFINE_CONSTANT(EAI_SOCKTYPE);
  DEFINE_CONSTANT(EAI_SYSTEM);
#ifdef EAI_BADHINTS
  DEFINE_CONSTANT(EAI_BADHINTS);
#endif
#ifdef EAI_PROTOCOL
  DEFINE_CONSTANT(EAI_PROTOCOL);
#endif
  DEFINE_CONSTANT(EAI_OVERFLOW);
}

void luaopen_lev_dns(lua_State *L) {
  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  define_constants(L);
  lua_setfield(L, -2, "dns");
}
