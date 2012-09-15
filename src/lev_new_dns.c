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
#include <arpa/inet.h>
/* #include "../src/ares/inet_net_pton.h" */
#include <errno.h>

#include <lua.h>
#include <lauxlib.h>
#include "luv_debug.h"

/*

  X:TODO Beginning of DNS Module...
  
*/

typedef struct dns_req_holder_s {
  LEVBASE_REF_FIELDS
  lua_State *L;
} dns_req_holder_t;

#define UNWRAP(h) \
  dns_req_holder_t *holder = (dns_req_holder_t *)(h); \
  lua_State* L = holder->L;

#define DNSR__CBNAME "_cb"

#define DNSR__SETUP \
  dns_req_holder_t *holder = (dns_req_holder_t *)create_obj_init_ref(L, \
      sizeof(dns_req_holder_t), "lev.dns");                             \
  holder->L = L;

#define DNSR__SET_CB(index) \
  /* NOTE: set_call needs "object" to be stack at index 1 */    \
  lua_insert(L, 1);                                             \
  set_callback(L, DNSR__CBNAME, (index+1));                     \
  lev_handle_ref(L, (LevRefStruct_t *)holder, 1);

#define DNSR__TEARDOWN \
  /* NOTE: remove "object" */                                   \
  lua_remove(L, 1);                                             \
  lua_pushnil(L);                                               \
  return 1;

#define DNSR__DISPOSE \
  lev_handle_unref(L, (LevRefStruct_t *)holder);

#define LEV_ARES_ERRNO_MAP(XX) \
  XX(ARES_SUCCESS, SUCCESS) \
  XX(ARES_ENODATA, ENODATA) \
  XX(ARES_EFORMERR, EFORMERR) \
  XX(ARES_ESERVFAIL, ESERVFAIL) \
  XX(ARES_ENOTFOUND, ENOTFOUND) \
  XX(ARES_ENOTIMP, ENOTIMP) \
  XX(ARES_EREFUSED, EREFUSED) \
  XX(ARES_EBADQUERY, EBADQUERY) \
  XX(ARES_EBADNAME, EBADNAME) \
  XX(ARES_EBADFAMILY, EBADFAMILY) \
  XX(ARES_EBADRESP, EBADRESP) \
  XX(ARES_ECONNREFUSED, ECONNREFUSED) \
  XX(ARES_ETIMEOUT, ETIMEOUT) \
  XX(ARES_EOF, EOF) \
  XX(ARES_EFILE, EFILE) \
  XX(ARES_ENOMEM, ENOMEM) \
  XX(ARES_EDESTRUCTION, EDESTRUCTION) \
  XX(ARES_EBADSTR, EBADSTR) \
  XX(ARES_EBADFLAGS, EBADFLAGS) \
  XX(ARES_ENONAME, ENONAME) \
  XX(ARES_EBADHINTS, EBADHINTS) \
  XX(ARES_ENOTINITIALIZED, ENOTINITIALIZED) \
  XX(ARES_ELOADIPHLPAPI, ELOADIPHLPAPI) \
  XX(ARES_EADDRGETNETWORKPARAMS, EADDRGETNETWORKPARAMS) \
  XX(ARES_ECANCELLED, ECANCELLED) \

#define LEV_ARES_ERR_CASE_GEN(val, name) \
  case val: return #name;

static const char *to_ares_errname(int errcode) {
  switch (errcode) {
  LEV_ARES_ERRNO_MAP(LEV_ARES_ERR_CASE_GEN)
  default: return "UNKNOWN";
  }
}

static void on_resolve4(void *arg, int status, int timeouts,
    struct hostent *host) {
  UNWRAP((dns_req_holder_t *)arg);
  int ret_n;

  push_callback_no_obj(L, holder, DNSR__CBNAME);
  if (status != ARES_SUCCESS) {
    ret_n = 1;
    lua_pushstring(L, to_ares_errname(status));
  } else {
    char **p;
    int i;
    char addr_buf[sizeof("255.255.255.255")];
    ret_n = 2;
    lua_pushnil(L);
    lua_newtable(L);
    for (p = host->h_addr_list, i = 1; *p; p++, i++) {
      inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
      lua_pushstring(L, addr_buf);
      lua_rawseti(L, -2, i);
    }
  }
  lua_call(L, ret_n, 0);

  DNSR__DISPOSE
}

static int dns_resolve4(lua_State* L) {
  DNSR__SETUP
  const char *domain = luaL_checkstring(L, 1);
  DNSR__SET_CB(2)
  ares_channel channel = lev_get_ares_channel(L);
  ares_gethostbyname(channel, domain, AF_INET, on_resolve4, holder);
  DNSR__TEARDOWN
}

static void on_resolve6(void *arg, int status, int timeouts,
    struct hostent *host) {
  UNWRAP((dns_req_holder_t *)arg);
  int ret_n;

  push_callback_no_obj(L, holder, DNSR__CBNAME);
  if (status != ARES_SUCCESS) {
    ret_n = 1;
    lua_pushstring(L, to_ares_errname(status));
  } else {
    char **p;
    int i;
    char addr_buf[sizeof("0000:0000:0000:0000:0000:0000:0000:0001")];
    ret_n = 2;
    lua_pushnil(L);
    lua_newtable(L);
    for (p = host->h_addr_list, i = 1; *p; p++, i++) {
      inet_ntop(host->h_addrtype, *p, addr_buf, sizeof(addr_buf));
      lua_pushstring(L, addr_buf);
      lua_rawseti(L, -2, i);
    }
  }
  lua_call(L, ret_n, 0);

  DNSR__DISPOSE
}

static int dns_resolve6(lua_State* L) {
  DNSR__SETUP
  const char *domain = luaL_checkstring(L, 1);
  DNSR__SET_CB(2)
  ares_channel channel = lev_get_ares_channel(L);
  ares_gethostbyname(channel, domain, AF_INET6, on_resolve6, holder);
  DNSR__TEARDOWN
}

static void on_reverse(void *arg, int status, int timeouts,
    struct hostent *host) {
  UNWRAP((dns_req_holder_t *)arg);
  int ret_n;

  push_callback_no_obj(L, holder, DNSR__CBNAME);
  if (status != ARES_SUCCESS) {
    ret_n = 1;
    lua_pushstring(L, to_ares_errname(status));
  } else {
    char **p;
    int i;
    ret_n = 2;
    lua_pushnil(L);
    lua_newtable(L);
    for (p = host->h_addr_list, i = 1; *p; p++, i++) {
      lua_pushstring(L, host->h_name);
      lua_rawseti(L, -2, i);
    }
  }
  lua_call(L, ret_n, 0);

  DNSR__DISPOSE
}

static int dns_reverse(lua_State* L) {
  DNSR__SETUP
  const char *ip = luaL_checkstring(L, 1);
  DNSR__SET_CB(2)
  assert(sizeof(struct in6_addr) >= sizeof(struct in_addr));
  char addr_buf[sizeof(struct in6_addr)];
  int addr_len;
  int family;
  if (inet_pton(AF_INET, ip, &addr_buf) == 1) {
    addr_len = sizeof(struct in_addr);
    family = AF_INET;
  } else if (inet_pton(AF_INET6, ip, &addr_buf) == 1) {
    addr_len = sizeof(struct in6_addr);
    family = AF_INET6;
  } else {
    lua_pushstring(L, "ENOTIMP");
    return 1;
  }

  ares_channel channel = lev_get_ares_channel(L);
  ares_gethostbyaddr(channel, addr_buf, addr_len, family, on_reverse, holder);
  DNSR__TEARDOWN
}


static luaL_reg functions[] = {
   { "resolve4",   dns_resolve4    }
  ,{ "resolve6",   dns_resolve6    }
  ,{ "reverse",    dns_reverse     }
  ,{ NULL, NULL }
};


void luaopen_lev_dns(lua_State *L) {
  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  lua_setfield(L, -2, "dns");
}
