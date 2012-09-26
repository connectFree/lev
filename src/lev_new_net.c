/*
 *  Copyright 2012 connectFree k.k. and the lev authors. All Rights Reserved.
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

#include <arpa/inet.h>

#include <lua.h>
#include <lauxlib.h>
#include "luv_debug.h"

/*

  X:TODO Beginning of NET Module...
  
*/

static int net_is_ipv4(lua_State* L) {
  const char *ip = luaL_checkstring(L, 1);
  struct in_addr addr;
  int r = inet_pton(AF_INET, ip, &addr);
  lua_pushboolean(L, r == 1);
  return 1;
}

static int net_is_ipv6(lua_State* L) {
  const char *ip = luaL_checkstring(L, 1);
  struct in_addr addr;
  int r = inet_pton(AF_INET6, ip, &addr);
  lua_pushboolean(L, r == 1);
  return 1;
}

static luaL_reg functions[] = {
   { "isIPv4",   net_is_ipv4    }
  ,{ "isIPv6",   net_is_ipv6    }
  ,{ NULL, NULL }
};


void luaopen_lev_net(lua_State *L) {
  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  lua_setfield(L, -2, "net");
}
