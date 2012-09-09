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

  X:TODO Beginning of UDP Module...
  
*/

/* TODO: remove this when maglev_fs pull request is merged. */
static void lev_push_uv_err(lua_State *L, uv_err_t err) {
  lua_createtable(L, 0, 2);

  lua_pushstring(L, uv_strerror(err));
  lua_setfield(L, -2, "message");

  lua_pushnumber(L, err.code);
  lua_setfield(L, -2, "code");
}

static MemBlock *_static_mb = NULL;

#define UNWRAP(h) \
  udp_obj* self = container_of((h), udp_obj, handle); \
  lua_State* L = self->handle.loop->data;

#define UV_UDP_CLOSE(handle)                          \
    uv_close((uv_handle_t *)handle, udp_after_close);

typedef struct {
  LEVBASE_REF_FIELDS
  uv_udp_t handle;
} udp_obj;

static int udp_new(lua_State* L) {
  uv_loop_t* loop;
  udp_obj* self;
  int r;

  loop = uv_default_loop();
  assert(L == loop->data);

  self = (udp_obj*)create_obj_init_ref(L, sizeof *self, "lev.udp");
  r = uv_udp_init(loop, &self->handle);
  assert(r == 0);

  return 1;
}

static int udp_bind(lua_State* L) {
  udp_obj* self;
  const char* host;
  int port;
  struct sockaddr_in addr;
  int r;

  self = luaL_checkudata(L, 1, "lev.udp");
  host = luaL_checkstring(L, 2);
  port = luaL_checkint(L, 3);

  addr = uv_ip4_addr(host, port);

  r = uv_udp_bind(&self->handle, addr, 0);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(luv_get_loop(L)));
    return 1;
  }
  lua_pushnil(L);
  lev_handle_ref(L, (LevRefStruct_t*)self, 1);

  return 1;
}

static int udp_bind6(lua_State* L) {
  udp_obj* self;
  const char* host;
  int port;
  int flags;
  struct sockaddr_in6 addr;
  int r;

  self = luaL_checkudata(L, 1, "lev.udp");
  host = luaL_checkstring(L, 2);
  port = luaL_checkint(L, 3);
  flags = luaL_optint(L, 4, 0);

  addr = uv_ip6_addr(host, port);

  r = uv_udp_bind6(&self->handle, addr, flags);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(luv_get_loop(L)));
    return 1;
  }
  lua_pushnil(L);
  lev_handle_ref(L, (LevRefStruct_t*)self, 1);

  return 1;
}

static void udp_after_send(uv_udp_send_t* req, int status) {
  UNWRAP(req->handle);
  lev_handle_unref(L, (LevRefStruct_t*)self);
  free(req);
}

static int udp_send(lua_State* L) {
  udp_obj* self;
  const char* host;
  int port;
  struct sockaddr_in addr;
  uv_buf_t buf;
  int r;

  self = luaL_checkudata(L, 1, "lev.udp");
  host = luaL_checkstring(L, 2);
  port = luaL_checkint(L, 3);

  if (lua_isstring(L, 4)) {
    size_t len;
    const char* chunk = luaL_checklstring(L, 4, &len);
    buf = uv_buf_init((char*)chunk, len);
  } else {
    /* TODO: isbuffer check */
    buf = lev_buffer_to_uv(L, 4);
  }  

  addr = uv_ip4_addr(host, port);

  uv_udp_send_t* req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
  r = uv_udp_send(req, &self->handle, &buf, 1, addr,
    udp_after_send);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(luv_get_loop(L)));
    return 1;
  }
  lua_pushnil(L);
  lev_handle_ref(L, (LevRefStruct_t*)self, 1);

  return 1;
}

/* TODO: Fix code duplication copied from lev_new_tcp.c */
static uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size) {
  uv_buf_t buf;
  size_t remaining;

  if (!_static_mb) {
    _static_mb = lev_slab_getBlock( suggested_size );
    lev_slab_incRef( _static_mb );
  }

  remaining = _static_mb->size - _static_mb->nbytes;
  if (_static_mb->size - _static_mb->nbytes < 512) {
    lev_slab_decRef( _static_mb );
    _static_mb = lev_slab_getBlock( suggested_size );
    lev_slab_incRef( _static_mb );
    remaining = _static_mb->size - _static_mb->nbytes;
  }

  buf.base = (char *)(_static_mb->bytes + _static_mb->nbytes);
  buf.len = remaining;

  return buf;
}

static void udp_after_close(uv_handle_t* handle) {
  UNWRAP(handle);
  if (push_callback(L, self, "on_close")) {
    lua_call(L, 1, 0);/*, -3*/
    
  }
  lev_handle_unref(L, (LevRefStruct_t*)self);
}

static int push_sockaddr(lua_State *L, struct sockaddr *addr) {
  char addr_buf[sizeof "255.255.255.255"];
  inet_ntop4(addr->sa_data + 2, addr_buf, sizeof(addr_buf));
  lua_pushstring(L, addr_buf);

  int port = ((unsigned)addr->sa_data[0] << 8) | ((unsigned)addr->sa_data[1]);
  lua_pushinteger(L, port);

  return 2;
}

static void on_recv(uv_udp_t* handle, ssize_t nread, uv_buf_t buf,
    struct sockaddr* addr, unsigned flags) {
  int r;
  int port;

  UNWRAP(handle);

  if (nread <= 0) {
    /* automatically close on error and EOF */
    UV_UDP_CLOSE(handle);

    if (nread == -1) {
      push_callback(L, self, "on_recv");
      lev_push_uv_err(L, uv_last_error(luv_get_loop(L)));
      lua_call(L, 2, 0);
    }
    return;
  }

  /* (This is copied from "test/test-udp-send-and-recv.c" in libuv.)
   *
   * FIXME? `uv_udp_recv_stop` does what it says: recv_cb is not called
   * anymore. That's problematic because the read buffer won't be returned
   * either... Not sure I like that but it's consistent with `uv_read_stop`.
   */
  r = uv_udp_recv_stop(handle);
  assert(r == 0);

  push_callback(L, self, "on_recv");

  lua_pushnil(L);
  push_sockaddr(L, addr);
  lua_pushinteger(L, nread);

  /* push new cBuffer */
  lev_pushbuffer_from_mb(
       L
      ,_static_mb
      ,nread
      ,_static_mb->bytes + _static_mb->nbytes
    ); /* automatically incRef's mb */
  _static_mb->nbytes += nread; /* consume nread bytes */

  lua_call(L, 6, 0);
}

static int udp_recv_start(lua_State* L) {
  udp_obj* self;
  int r;

  self = luaL_checkudata(L, 1, "lev.udp");
  set_callback(L, "on_recv", 2);

  r = uv_udp_recv_start(&self->handle, on_alloc, on_recv);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(luv_get_loop(L)));
    return 1;
  }

  lua_pushnil(L);
  lev_handle_ref(L, (LevRefStruct_t*)self, 1);

  return 1;
}

static int udp_close(lua_State* L) {
  udp_obj* self;

  self = luaL_checkudata(L, 1, "lev.udp");

  if (lua_isfunction(L, 2))
    set_callback(L, "on_close", 2);

  UV_UDP_CLOSE(&self->handle);

  return 0;
}

static int udp_rcb_close(lua_State* L) {
  luaL_checkudata(L, 1, "lev.udp"); /* we won't use the data, but check if it is actually lev.udp*/
  set_callback(L, "on_close", 2);
  return 0;
}

static int udp_getsockname(lua_State* L) {
  udp_obj* self;
  struct sockaddr name;
  int namelen = sizeof(name);

  self = luaL_checkudata(L, 1, "lev.udp");
  int r = uv_udp_getsockname(&self->handle, &name, &namelen);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(luv_get_loop(L)));
    return 1;
  }

  lua_pushnil(L);
  push_sockaddr(L, &name);
  return 3;
}

static int udp_set_multicast_loop(lua_State* L) {
  udp_obj* self;

  self = luaL_checkudata(L, 1, "lev.udp");
  int on = lua_toboolean(L, 2);
  int r = uv_udp_set_multicast_loop(&self->handle, on);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(luv_get_loop(L)));
    return 1;
  }

  lua_pushnil(L);
  return 1;
}

static int udp_set_multicast_ttl(lua_State* L) {
  udp_obj* self;

  self = luaL_checkudata(L, 1, "lev.udp");
  int ttl = luaL_checkint(L, 2);
  int r = uv_udp_set_multicast_ttl(&self->handle, ttl);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(luv_get_loop(L)));
    return 1;
  }

  lua_pushnil(L);
  return 1;
}

static int udp_set_broadcast(lua_State* L) {
  udp_obj* self;

  self = luaL_checkudata(L, 1, "lev.udp");
  int on = lua_toboolean(L, 2);
  int r = uv_udp_set_broadcast(&self->handle, on);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(luv_get_loop(L)));
    return 1;
  }

  lua_pushnil(L);
  return 1;
}

static int udp_set_membership(lua_State* L) {
  udp_obj* self;

  self = luaL_checkudata(L, 1, "lev.udp");
  const char *multicast_addr = luaL_optstring(L, 2, NULL);
  const char *interface_addr = luaL_optstring(L, 3, NULL);
  int membership = luaL_checkint(L, 4);
  int r = uv_udp_set_membership(&self->handle, multicast_addr, interface_addr,
      membership);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(luv_get_loop(L)));
    return 1;
  }

  lua_pushnil(L);
  return 1;
}

static int udp_set_ttl(lua_State* L) {
  udp_obj* self;

  self = luaL_checkudata(L, 1, "lev.udp");
  int ttl = luaL_checkint(L, 2);
  int r = uv_udp_set_ttl(&self->handle, ttl);
  if (r == -1) {
    lev_push_uv_err(L, uv_last_error(luv_get_loop(L)));
    return 1;
  }

  lua_pushnil(L);
  return 1;
}

static luaL_reg methods[] = {
   { "bind",               udp_bind               }
  ,{ "bind6",              udp_bind6              }
  ,{ "close",              udp_close              }
  ,{ "on_close",           udp_rcb_close          }
  ,{ "getsockname",        udp_getsockname        }
  ,{ "recv_start",         udp_recv_start         }
  ,{ "send",               udp_send               }
  ,{ "set_broadcast",      udp_set_broadcast      }
  ,{ "set_multicast_loop", udp_set_multicast_loop }
  ,{ "set_multicast_ttl",  udp_set_multicast_ttl  }
  ,{ "set_membership",     udp_set_membership     }
  ,{ "set_ttl",            udp_set_ttl            }
  ,{ NULL,                 NULL                   }
};


static luaL_reg functions[] = {
   { "new", udp_new }
  ,{ NULL, NULL }
};


void luaopen_lev_udp(lua_State *L) {
  luaL_newmetatable(L, "lev.udp");
  luaL_register(L, NULL, methods);
  lua_setfield(L, -1, "__index");

  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  lua_setfield(L, -2, "udp");
}
