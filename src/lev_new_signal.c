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

#include <lua.h>
#include <lauxlib.h>
#include "luv_debug.h"

void luaopen_lev_signal(lua_State *L) {
  lua_newtable(L);

  LEV_SET_FIELD(SIGPIPE, number, SIGPIPE);

  lua_setfield(L, -2, "signal");
}
