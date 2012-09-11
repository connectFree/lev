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


static MemBlock *_static_mb = NULL;

#define BUFFER_SIZE 8*1024

#define UNWRAP(h) \
  tcp_obj* self = container_of((h), tcp_obj, handle); \
  lua_State* L = self->handle.loop->data;

#define UV_CLOSE_CLIENT                           \
    uv_read_stop((uv_stream_t*)&self->handle);    \
    uv_shutdown_t* shutdown_req;                  \
    shutdown_req = malloc(sizeof(uv_shutdown_t)); \
    lev_handle_ref(L, (LevRefStruct_t*)self, 1);  \
    uv_shutdown(                                  \
      shutdown_req                                \
      ,(uv_stream_t*)&self->handle                \
      ,tcp_after_shutdown                         \
    );                                            \


typedef struct {
  LEVBASE_REF_FIELDS
  uv_tcp_t handle;
  uv_connect_t connect_req; /* TODO alloc on as needed basis */
} tcp_obj;


static void tcp_after_close(uv_handle_t* handle) {
  UNWRAP(handle);
  if (push_callback(L, self, "on_close")) {
    lua_call(L, 1, 0);/*, -3*/
    
  }
  lev_handle_unref(L, (LevRefStruct_t*)self);
}

void tcp_after_shutdown(uv_shutdown_t* req, int status) {
  UNWRAP(req->handle);
  lev_handle_unref(L, (LevRefStruct_t*)self);
  uv_close((uv_handle_t*)&self->handle, tcp_after_close);
  free(req);
}

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


static void on_connect(uv_connect_t* req, int status) {
  UNWRAP(req->handle);
  lev_handle_unref(L, (LevRefStruct_t*)self);
  push_callback(L, self, "on_connect");
  if (!status) {
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, status);
  }
  lua_call(L, 2, 0);/*, -4*/
}


static void on_connection(uv_stream_t* handle, int status) {
  UNWRAP(handle);
  push_callback(L, self, "on_connection");
  if (!status) {
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, status);
  }
  lua_call(L, 2, 0);/*, -4*/
}

static void on_read(uv_stream_t* handle, ssize_t nread, uv_buf_t buf) {
  UNWRAP(handle);
  if (-1 == nread) {/* automatically shutdown connection */
    UV_CLOSE_CLIENT
  } else {
    push_callback(L, self, "on_read");
    lua_pushinteger(L, nread);

    /* push new cBuffer */
    lev_pushbuffer_from_mb(
         L
        ,_static_mb
        ,nread
        ,_static_mb->bytes + _static_mb->nbytes
      ); /* automatically incRef's mb */
    _static_mb->nbytes += nread; /* consume nread bytes */

    lua_call(L, 3, 0);/*, -5*/
  }
}

static int tcp_new(lua_State* L) {
  uv_loop_t* loop;
  tcp_obj* self;
  int use_this_fd;

  use_this_fd = lua_tointeger(L, 1);

  loop = uv_default_loop();
  assert(L == loop->data);

  self = (tcp_obj*)create_obj_init_ref(L, sizeof *self, "lev.tcp");
  uv_tcp_init(loop, &self->handle);

  if (use_this_fd) {
    self->handle.fd = use_this_fd;
  }

  return 1;
}


static int tcp_accept(lua_State* L) {
  tcp_obj* self;
  tcp_obj* obj;
  int r;

  self = luaL_checkudata(L, 1, "lev.tcp");

  obj = (tcp_obj*)create_obj_init_ref(L, sizeof *obj, "lev.tcp");
  uv_tcp_init(self->handle.loop, &obj->handle);

  r = uv_accept((uv_stream_t*)&self->handle,
                (uv_stream_t*)&obj->handle);
  if (!r) {
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, r);
  }
  return 2;
}


static int tcp_bind(lua_State* L) {
  struct sockaddr_in addr;
  const char* host;
  tcp_obj* self;
  int port;
  int r;

  self = luaL_checkudata(L, 1, "lev.tcp");
  host = luaL_checkstring(L, 2);
  port = luaL_checkint(L, 3);

  addr = uv_ip4_addr(host, port);

  r = uv_tcp_bind(&self->handle, addr);
  if (!r) {
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, r);
  }
  lev_handle_ref(L, (LevRefStruct_t*)self, 1);

  return 1;
}


static int tcp_connect(lua_State* L) {
  struct sockaddr_in addr;
  const char* host;
  tcp_obj* self;
  int port;
  int r;

  self = luaL_checkudata(L, 1, "lev.tcp");
  host = luaL_checkstring(L, 2);
  port = luaL_checkint(L, 3);
  set_callback(L, "on_connect", 4);

  addr = uv_ip4_addr(host, port);

  r = uv_tcp_connect(&self->connect_req, &self->handle, addr, on_connect);
  if (!r) {
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, r);
  }
  lev_handle_ref(L, (LevRefStruct_t*)self, 1);

  return 1;
}


static int tcp_close(lua_State* L) {
  tcp_obj* self;

  self = luaL_checkudata(L, 1, "lev.tcp");

  if (lua_isfunction(L, 2))
    set_callback(L, "on_close", 2);

  UV_CLOSE_CLIENT

  return 0;
}

static int tcp_fd_get(lua_State* L) {
  tcp_obj* self;

  self = luaL_checkudata(L, 1, "lev.tcp");

  lua_pushinteger(L, self->handle.fd);
  return 1;
}

static int tcp_fd_set(lua_State* L) {
  tcp_obj* self;

  self = luaL_checkudata(L, 1, "lev.tcp");

  self->handle.fd = luaL_checkint(L, 2);

  return 0;
}

static int tcp_listen(lua_State* L) {
  tcp_obj* self;
  int backlog;
  int r;

  self = luaL_checkudata(L, 1, "lev.tcp");

  if (lua_isnumber(L, 2)) {
    backlog = luaL_checkinteger(L, 2);
    set_callback(L, "on_connection", 3);
  }
  else {
    backlog = 128;
    set_callback(L, "on_connection", 2);
  }

  r = uv_listen((uv_stream_t*)&self->handle, backlog, on_connection);
  if (!r) {
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, r);
  }

  lev_handle_ref(L, (LevRefStruct_t*)self, 1);

  return 1;
}

static int tcp_rcb_close(lua_State* L) {
  luaL_checkudata(L, 1, "lev.tcp"); /* we won't use the data, but check if it is actually lev.tcp*/
  set_callback(L, "on_close", 2);
  return 0;
}

static int tcp_read_start(lua_State* L) {
  tcp_obj* self;
  int r;

  self = luaL_checkudata(L, 1, "lev.tcp");
  set_callback(L, "on_read", 2);

  r = uv_read_start((uv_stream_t*)&self->handle, on_alloc, on_read);
  if (!r) {
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, r);
  }

  lev_handle_ref(L, (LevRefStruct_t*)self, 1);

  return 1;
}


static int tcp_read_stop(lua_State* L) {
  tcp_obj* self;
  int r;

  self = luaL_checkudata(L, 1, "lev.tcp");

  r = uv_read_stop((uv_stream_t*)&self->handle);
  if (!r) {
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, r);
  }

  if (r == 0)
    clear_callback(L, "on_read", self);

  return 1;
}

void tcp_after_write(uv_write_t* req, int status) {
  UNWRAP(req->handle);
  lev_handle_unref(L, (LevRefStruct_t*)self);
  free(req);
}


static int tcp_write(lua_State* L) {
  tcp_obj* self;
  uv_buf_t buf;
  size_t len;

  self = luaL_checkudata(L, 1, "lev.tcp");

  if (lua_isstring(L, 2)) {
    const char* chunk = luaL_checklstring(L, 2, &len);
    buf = uv_buf_init((char*)chunk, len);
  } else {
    buf = lev_buffer_to_uv(L, 2);
  }  

  uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
  uv_write(req, (uv_stream_t*)&self->handle, &buf, 1, tcp_after_write);
  lev_handle_ref(L, (LevRefStruct_t*)self, 1);

  return 0;
}

static int tcp_nodelay(lua_State* L) {
  tcp_obj* self;

  self = luaL_checkudata(L, 1, "lev.tcp");

  uv_tcp_nodelay((uv_tcp_t*)&self->handle, luaL_checkint(L, 2));

  return 0;
}



static luaL_reg methods[] = {
   { "accept",     tcp_accept      }
  ,{ "bind",       tcp_bind        }
  ,{ "connect",    tcp_connect     }
  ,{ "close",      tcp_close       }
  ,{ "listen",     tcp_listen      }
  ,{ "on_close",   tcp_rcb_close   }
  ,{ "read_start", tcp_read_start  }
  ,{ "read_stop",  tcp_read_stop   }
  ,{ "write",      tcp_write       }
  ,{ "fd_get",     tcp_fd_get      }
  ,{ "fd_set",     tcp_fd_set      }
  ,{ "nodelay",    tcp_nodelay     }
  ,{ NULL,         NULL            }
};


static luaL_reg functions[] = {
   { "new", tcp_new }
  /*,{ "newServer", tcp_new_server }*/
  ,{ NULL, NULL }
};


void luaopen_lev_tcp(lua_State *L) {
  luaL_newmetatable(L, "lev.tcp");
  luaL_register(L, NULL, methods);
  lua_setfield(L, -1, "__index");

  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  lua_setfield(L, -2, "tcp");
}