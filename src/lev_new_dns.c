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

typedef struct dns_req_holder_s {
  LEVBASE_REF_FIELDS
  int getnameinfo_flags;
  uv_getaddrinfo_t req;
} dns_req_holder_t;

#define UNWRAP(r) \
  dns_req_holder_t* holder = container_of((r), dns_req_holder_t, req); \
  lua_State* L = (r)->loop->data;

#define DNSR__CBNAME "_cb"

#define DNSR__SETUP \
  uv_getaddrinfo_cb cb = NULL;                                          \
  uv_loop_t *loop = luv_get_loop(L);                                    \
  dns_req_holder_t *holder = (dns_req_holder_t *)create_obj_init_ref(L, \
      sizeof(dns_req_holder_t), "lev.dns");                             \
  /* NOTE: set_call needs "object" to be stack at index 1 */            \
  lua_insert(L, 1);                                                     \
  uv_getaddrinfo_t *req = &holder->req;

#define DNSR__SET_CB(index, c_callback) \
  set_callback(L, DNSR__CBNAME, (index));                       \
  lev_handle_ref(L, (LevRefStruct_t *)holder, -1);              \
  cb = (c_callback);                                            \

#define DNSR__TEARDOWN \
  /* NOTE: remove "object" */                                   \
  lua_remove(L, 1);                                             \
  if (r) {                                                      \
    lev_push_uv_err(L, lev_code_to_uv_err(r));                  \
    return 1;                                                   \
  }                                                             \
  return 0;

static void on_getaddrinfo_callback(uv_getaddrinfo_t *req, int status,
    struct addrinfo *res) {
  UNWRAP(req)
  int arg_n;

  push_callback_no_obj(L, holder, DNSR__CBNAME);
  if (status == -1) {
    arg_n = 1;
    lev_push_uv_err(L, uv_last_error(req->loop));
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
        LEV_SET_FIELD(code, number, r);
        LEV_SET_FIELD(message, string, gai_strerror(r));
      }

      lua_newtable(L);
      LEV_SET_FIELD(flags, number, ai->ai_flags);
      LEV_SET_FIELD(family, number, ai->ai_family);
      LEV_SET_FIELD(socktype, number, ai->ai_socktype);
      LEV_SET_FIELD(protocol, number, ai->ai_protocol);
      LEV_SET_FIELD(canonname, string, ai->ai_canonname);
      LEV_SET_FIELD(host, string, host_buf);
      LEV_SET_FIELD(serv, string, serv_buf);
      lua_rawseti(L, -2, i);
    }
  }
  lua_call(L, arg_n, 0);

  uv_freeaddrinfo(res);
  lev_handle_unref(L, (LevRefStruct_t *)holder);
}

static int dns_getaddrinfo(lua_State* L) {
  DNSR__SETUP
  /* NOTE: index is added by 1 because of holder above. */
  const char *host = luaL_optstring(L, 2, NULL);
  const char *serv = luaL_optstring(L, 3, NULL);

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = PF_UNSPEC;
  if (lua_istable(L, 4)) {
#define SET_ADDRINFO_FIELD(name) \
  lua_getfield(L, 4, #name); \
  if (lua_isnumber(L, -1)) { \
    hints.ai_##name = lua_tonumber(L, -1); \
  } else if (!lua_isnil(L, -1)) { \
    return luaL_argerror(L, 4, "field '##name' must be of type 'number'"); \
  } \
  lua_pop(L, 1);

    SET_ADDRINFO_FIELD(family);
    SET_ADDRINFO_FIELD(socktype);
    SET_ADDRINFO_FIELD(protocol);
    SET_ADDRINFO_FIELD(flags);
  } else if (!lua_isnil(L, 4)) {
    return luaL_argerror(L, 4, "Must be of type 'table' or nil");
  }

  holder->getnameinfo_flags = luaL_optint(L, 5, 0);
  DNSR__SET_CB(6, on_getaddrinfo_callback)

  int r = uv_getaddrinfo(loop, req, cb, host, serv, &hints);

  DNSR__TEARDOWN
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
