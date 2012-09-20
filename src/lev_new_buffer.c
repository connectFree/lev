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

#if defined(_MSC_VER)
/* Ah, MSVC */
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef __int32 intptr_t;
typedef unsigned __int32 uintptr_t;
#else
#include <stdint.h>
#endif

/* MSVC Built-in */
#if defined(_MSC_VER)
/* http://msdn.microsoft.com/en-us/library/a3140177.aspx */
#define __bswap16   _byteswap_ushort
#define __bswap32   _byteswap_ulong
/* GCC Built-in */
#elif (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
/* http://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html */

  /* NOOP */

/* x86 */
#elif defined(__i386__) || defined(__x86_64__)
static uint32_t __builtin_bswap32(uint32_t x)
{
  uint32_t r; __asm__("bswap %0" : "=r" (r) : "0" (x)); return r;
}
/* other platforms */
#else
static uint32_t __builtin_bswap32(uint32_t x)
{
  return (x << 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00) | (x >> 24);
}
#endif

#if !defined( _MSC_VER )
/* NON MSVC solutions require defining __builtin_bswap16 */
static uint16_t __builtin_bswap16( uint16_t a ) {
  return (a<<8)|(a>>8);
}
#define __bswap16   __builtin_bswap16
#define __bswap32   __builtin_bswap32
#endif

/******************************************************************************/

/* Mac OS X < 10.7.x has no "memmem" */
#if !((!defined(__GNUC__) || __GNUC__ >= 2) && !defined(__APPLE__))
#undef memmem
/* memmem implementation from fbsd */
/* Copyright (c) 2005 Pascal Gloor <pascal.gloor@spale.com> */
/* http://opensource.apple.com/source/Libc/Libc-763.12/string/memmem-fbsd.c */
static void *my_memmem(l, l_len, s, s_len)
  const void *l; size_t l_len;
  const void *s; size_t s_len;
{
  register char *cur, *last;
  const char *cl = (const char *)l;
  const char *cs = (const char *)s;

  /* we need something to compare */
  if (l_len == 0 || s_len == 0) {
    return NULL;
  }

  /* "s" must be smaller or equal to "l" */
  if (l_len < s_len) {
    return NULL;
  }

  /* special case where s_len == 1 */
  if (s_len == 1) {
    return memchr(l, (int)*cs, l_len);
  }

  /* the last position where its possible to find "s" in "l" */
  last = (char *)cl + l_len - s_len;

  for (cur = (char *)cl; cur <= last; cur++) {
    if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0) {
      return cur;
    }
  }

  return NULL;
}

#define memmem my_memmem

#endif

/******************************************************************************/

#define lua_boxpointer(L, u)   (*(void **)(lua_newuserdata(L, sizeof(void *))) = (u))
#define lua_unboxpointer(L, i)   (*(void **)(lua_touserdata(L, i)))
#define lua_unboxpointerchk(L, i, tname)   (*(void **)(luaL_checkudata(L, i, tname)))


#define BUFFER_UDATA(L)                                                        \
  MemSlice *ms = luaL_checkudata(L, 1, "lev.buffer");                          \
  unsigned char *buffer = (unsigned char *)ms->slice;                          \
  size_t buffer_len = ms->until;                                               \

#define _BUFFER_GETOFFSET(L, _idx, extbound)                                   \
  size_t index = (size_t)lua_tointeger(L, _idx);                                \
  if (index < 1 || index - 1 > buffer_len - extbound) {                        \
    return luaL_argerror(L, _idx, "Index out of bounds");                      \
  }

#define BUFFER_GETOFFSET(L, _idx) _BUFFER_GETOFFSET(L, _idx, 0)

static MemBlock *_static_mb = NULL;

/******************************************************************************/

MemSlice *lev_checkbufobj(lua_State *L, int index) {
  MemSlice *ms = lua_unboxpointerchk(L, index, "lev.buffer");
  return ms;
}

int lev_pushbuffer_from_mb(lua_State *L, MemBlock *mb, size_t until, unsigned char *slice) {
  MemSlice *ms;

  lev_slab_incRef( mb );

  ms = (MemSlice *)create_obj_init_ref(L, sizeof *ms, "lev.buffer");
  ms->mb = mb;
  ms->slice = (!slice ? mb->bytes : slice);
  ms->until = (!until ? (!mb->nbytes ? mb->size : mb->nbytes) : until);

  /*printf("lev_pushbuffer_from_mb: mb:%p;s:%p;u:%lu\n", ms->mb, ms->slice, ms->until);*/

  return 1;
}

uv_buf_t lev_buffer_to_uv(lua_State *L, int index) {
  MemSlice *ms = luaL_checkudata(L, index, "lev.buffer");
  return uv_buf_init((char*)ms->slice, ms->until);
}

MemSlice * lev_buffer_new(lua_State *L, size_t size, const char *temp, size_t temp_size) {
  MemBlock *mb = NULL;

  if (_static_mb && _static_mb->size - _static_mb->nbytes < size) {
    lev_slab_decRef( _static_mb );
    _static_mb = lev_slab_getBlock( size );
  } else if (!_static_mb){
    _static_mb = lev_slab_getBlock( size );
  }

  mb = _static_mb;
  if (size != _static_mb->size) {
    lev_slab_incRef( _static_mb );
  } else { /* we completely own this MemBlock, no need to give it to others */
    _static_mb = NULL;
  }

  lev_pushbuffer_from_mb(
     L
    ,mb
    ,size
    ,mb->bytes + mb->nbytes
  ); /* automatically incRef's mb */

  if (temp) {
    if (!temp_size) {
      temp_size = size;
    }
    memcpy(
      mb->bytes + mb->nbytes
      ,temp
      ,(size > temp_size ? temp_size : size)
      );
  } else {
    memset(mb->bytes + mb->nbytes, '\0', size);
  }

  mb->nbytes += size;

  /* return the userdata */
  return lua_touserdata(L, -1);
}

/* Return bytes remaining in MemBlock
 * Ensure there is space for a NULL terminator. */
size_t lev_memblock_empty_length(MemBlock *mb) {
  return mb->size - mb->nbytes - 1;
}

/* Return bytes remaining in MemSlice
 * Ensure there is space for a NULL terminator. */
size_t lev_memslice_empty_length(MemSlice *ms) {
  return ms->until - 1;
}

/* 
  size is the size of the memslice that we want,
  *not* the size some byterange that we are about to append
*/
void lev_memslice_resize(MemSlice *ms, size_t size) {
  MemBlock *mb_new;

  if (size <= lev_memslice_empty_length(ms)) {
    return;
  }

  printf("[%p] GOING FOR RESIZE!\n", ms);

  size++; /* compensate for terminating NULL */

  int needs_new_mb = 1;

  /* we need to check if we are able to expand on this MemBlock */
  if (ms->slice + ms->until == ms->mb->bytes + ms->mb->nbytes) {
    if (size <= ms->until + (ms->mb->size - ms->mb->nbytes)) {
      needs_new_mb = 0;
    }
  } 

  if (needs_new_mb) {
    mb_new = lev_slab_getBlock( size );
    lev_slab_incRef( mb_new );
    memcpy(
       mb_new->bytes
      ,ms->slice
      ,ms->until /* we can only increase the buffer at this point, so use ms->until over size */
    );
    ms->slice = mb_new->bytes;
    ms->until = size;
    lev_slab_decRef(ms->mb); /* gc */
    ms->mb = mb_new;
    ms->mb->nbytes += size;
  } else {
    /* cool, we can use the remaining length of our current MemBlock */
    ms->mb->nbytes += size;
    ms->until = size;
  }

  return;
}

void lev_memslice_ensure_empty_length(MemSlice *ms, int len) {
  if (len > lev_memslice_empty_length(ms)) {
    lev_memslice_resize(ms, len);
  }
}

void lev_memslice_append_char_unsafe(MemSlice *ms, size_t at, const char c) {
  ms->slice[at] = c;
}

void lev_memslice_ensure_null(MemSlice *ms, size_t at) {
  ms->slice[at] = 0;
}

void lev_memslice_append_char(MemSlice *ms, size_t at, const char c) {
  lev_memslice_ensure_empty_length(ms, at + 1);
  ms->slice[at] = c;
}

void lev_memslice_append_mem(MemSlice *ms, size_t from, const char *c, size_t len) {
  lev_memslice_ensure_empty_length(ms, from + len);
  memcpy(ms->slice + from, c, len);
}

char *lev_memslice_empty_ptr(MemSlice *ms, size_t from) {
  return (char *)(ms->slice + from);
}

void lev_memslice_append_mem_unsafe(MemSlice *ms, size_t from, const char *c, size_t len) {
  memcpy(ms->slice + from, c, len);
}


size_t lev_memslice_append_string(MemSlice *ms, size_t from, const char *str) {
  int i;
  size_t space;
  size_t _from;

  _from = from;

  space = lev_memslice_empty_length(ms);

  for (i = 0; str[i]; i++) {
    if (space < 1) {
      lev_memslice_resize(ms, i + 1);
      space = lev_memslice_empty_length(ms);
    }
    ms->slice[_from++] = str[i];
    space--;
  }
  return _from - from;
}

/******************************************************************************/

/* Takes as arguments a number or string */
static int buffer_new (lua_State *L) {
  int entry_type;
  size_t buffer_size;
  const char *lua_temp = NULL;

  int init_idx = 1;

  entry_type = lua_type(L, 1);
  if (LUA_TTABLE == entry_type) {
    init_idx++;
  }

  entry_type = lua_type(L, init_idx);
  if (LUA_TNUMBER == entry_type) { /* are we perscribing a length */
    buffer_size = (size_t)lua_tointeger(L, init_idx);
  } else if (LUA_TSTRING == entry_type) { /* must be a string */
    lua_temp = lua_tolstring(L, init_idx, &buffer_size);
  } else {
    return luaL_argerror(L, init_idx, "Must be of type 'Number' or 'String'");
  }

  lev_buffer_new(L, buffer_size, lua_temp, 0);

  /* return the userdata */
  return 1;
}


/* isBuffer( obj ) */
static int buffer_isbuffer (lua_State *L) {
  int is_buffer = 0;

  void*p=lua_touserdata(L, 1);
  if(p!=NULL){
    if(lua_getmetatable(L,1)){
      lua_getfield(L,(-10000),"lev.buffer");
      if(lua_rawequal(L,-1,-2)){
        lua_pop(L,2);
        is_buffer = 1;
      }
    }
  }

  lua_pushboolean(L, is_buffer);
  return 1;
}

/******************************************************************************/

/* tostring(buffer, i, j) */
static int buffer_tostring (lua_State *L) {
  BUFFER_UDATA(L)

  size_t offset = (size_t)lua_tointeger(L, 2);
  if (!offset) {
    offset = 1;
  }
  if (offset < 1 || offset > buffer_len) {
    return luaL_argerror(L, 2, "Offset out of bounds");
  }

  offset--; /* account for Lua-isms */

  size_t length = (size_t)lua_tointeger(L, 3);
  if (!length) {
    length = buffer_len - offset;
  }
  if (length > buffer_len - offset) {
    return luaL_argerror(L, 3, "length out of bounds");
  }

  /* skip first size_t length */
  lua_pushlstring(L, (const char *)buffer + offset, length);
  return 1;
}

/* slice(buffer, i, j) */
static int buffer_slice (lua_State *L) {
  MemSlice *slice_ms;

  MemSlice *ms = luaL_checkudata(L, 1, "lev.buffer");

  size_t offset = (size_t)lua_tointeger(L, 2);
  if (!offset) {
    offset = 1;
  }
  if (offset < 1 || offset > ms->until) {
    return luaL_argerror(L, 2, "Offset out of bounds");
  }

  offset--; /* account for Lua-isms */

  size_t length = (size_t)lua_tointeger(L, 3);
  if (!length) {
    length = ms->until - offset;
  }
  if (length > ms->until - offset) {
    return luaL_argerror(L, 3, "length out of bounds");
  }

  lev_slab_incRef( ms->mb );
  slice_ms = (MemSlice *)create_obj_init_ref(L, sizeof *slice_ms, "lev.buffer");
  slice_ms->mb = ms->mb;
  slice_ms->slice = ms->slice + offset;
  slice_ms->until = length;

  return 1;
}

/* fill(buffer, char, i, j) */
static int buffer_fill (lua_State *L) {
  BUFFER_UDATA(L)

  size_t offset = (size_t)lua_tointeger(L, 3);
  if (!offset) {
    offset = 1;
  }
  if (offset < 1 || offset > buffer_len) {
    return luaL_argerror(L, 3, "Offset out of bounds");
  }

  size_t length = (size_t)lua_tointeger(L, 4);
  if (!length) {
    length = buffer_len;
  }
  if (length < offset || length > buffer_len) {
    return luaL_argerror(L, 4, "length out of bounds");
  }

  offset--; /* account for Lua-isms */

  uint8_t value;
  int entry_type;

  entry_type = lua_type(L, 2);

  if (LUA_TNUMBER == entry_type) {
    value = (uint8_t)lua_tointeger(L, 2); /* using value_len here as value */
    if (value > 255) {
      return luaL_argerror(L, 2, "0 < Value < 255");
    }
  } else if (LUA_TSTRING == entry_type) {
    value = *((uint8_t*)lua_tolstring(L, 2, NULL));
  } else {
    return luaL_argerror(L, 2, "Value is not of type 'String' or 'Number'");
  }

  memset(buffer + offset, value, length - offset);
  return 0;
}

/* find(buffer, char) */
static int buffer_find (lua_State *L) {
  size_t delimiter_len;
  const char *delimiter_buf = NULL;
  const char *found = NULL;

  BUFFER_UDATA(L)

  delimiter_buf = lua_tolstring(L, 2, &delimiter_len);
  if (!delimiter_buf) {
    return luaL_argerror(L, 2, "String is Required");
  }

  if (!delimiter_len) {
    lua_pushnil(L);
    return 1;
  }

  /* use memmem to find first instance of our delimiter */
  found = memmem(
             (const char *)buffer
            ,buffer_len
            ,delimiter_buf
            ,delimiter_len
          );

  if (!found) {
    lua_pushnil(L);
  } else {
    lua_pushnumber(L, (size_t)(found - (const char *)buffer) + 1);
  }

  return 1;
}

static const char _hex[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

/* inspect(buffer) */

static int buffer_inspect (lua_State *L) {
  int i;

  BUFFER_UDATA(L)

  size_t str_len = 8 /* <Buffer */
                 + buffer_len*3 /* buf_len hex+space */
                 + 1 /* > */
                 ;

  char *str = (char *)malloc(str_len);
  char *t = str;
  sprintf(str, "<Buffer ");
  t = t + 8;
  for(i=0;i<buffer_len;i++) {
    *((char *)t) = _hex[(buffer[i] >> 4)];
    *((char *)(t+1)) = _hex[(buffer[i] % 0x10)];
    *((char *)(t+2)) = ' ';
    t = t + 3;
  }
  *((char *)t) = '>';

  lua_pushlstring(L, str, str_len);
  free( str );
  return 1;
}

/* upuntil(buffer, i, j) */
static int buffer_upuntil (lua_State *L) {
  size_t delimiter_len;
  const char *delimiter_buf = NULL;
  const char *found = NULL;

  BUFFER_UDATA(L)

  delimiter_buf = lua_tolstring(L, 2, &delimiter_len);
  if (!delimiter_buf) {
    return luaL_argerror(L, 2, "Delimiter String is Required");
  }

  if (!delimiter_len) {
    lua_pushlstring(L, "", 0);
    return 1;
  }

  size_t offset = (size_t)lua_tointeger(L, 3);
  if (!offset) {
    offset = 1;
  }
  if (offset < 1 || offset > buffer_len) {
    return luaL_argerror(L, 3, "Offset out of bounds");
  }
  offset--; /* account for Lua-isms */

  /* use memmem to find first instance of our delimiter */
  found = memmem(
             (const char *)buffer + offset
            ,buffer_len - offset
            ,delimiter_buf
            ,delimiter_len
          );

  if (!found) { /* if nothing is found, return from offset */
    lua_pushlstring(L, (const char *)buffer + offset, buffer_len - offset);
  } else {
    lua_pushlstring(L, 
       (const char *)buffer + offset
      ,found - ((const char *)buffer) - offset
    );
  }
  return 1;
}

/* readInt8(buffer, offset) */
static int buffer_readInt8 (lua_State *L) {
  BUFFER_UDATA(L)

  BUFFER_GETOFFSET(L, 2) /* sets index */

  lua_pushnumber(L, *((int8_t*)(buffer + index - 1)));
  return 1;
}

/* readUInt16LE(buffer, offset) */
static int buffer_readUInt16BE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 2, 2) /* sets index */

  lua_pushnumber(L, __bswap16( *((uint16_t*)(buffer + index - 1)) ) );
  return 1;
}

/* readUInt16LE(buffer, offset) */
static int buffer_readUInt16LE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 2, 2) /* sets index */

  lua_pushnumber(L, *((uint16_t*)(buffer + index - 1)));
  return 1;
}

/* readUInt32LE(buffer, offset) */
static int buffer_readUInt32BE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 2, 4) /* sets index */

  lua_pushnumber(L, __bswap32( *((uint32_t*)(buffer + index - 1)) ) );
  return 1;
}


/* readUInt32LE(buffer, offset) */
static int buffer_readUInt32LE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 2, 4) /* sets index */

  lua_pushnumber(L, *((uint32_t*)(buffer + index - 1)));
  return 1;
}

/* readInt32LE(buffer, offset) */
static int buffer_readInt32BE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 2, 4) /* sets index */

  lua_pushnumber(L, (int32_t)__bswap32( *((uint32_t*)(buffer + index - 1)) ) );
  return 1;
}


/* readInt32LE(buffer, offset) */
static int buffer_readInt32LE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 2, 4) /* sets index */

  lua_pushnumber(L, *((int32_t*)(buffer + index - 1)));
  return 1;
}

/* writeUInt8(buffer, value, offset) */
static int buffer_writeUInt8 (lua_State *L) {
  BUFFER_UDATA(L)

  BUFFER_GETOFFSET(L, 3) /* sets index */

  *((uint8_t*)(buffer + index - 1)) = luaL_checknumber(L, 2);

  return 0;
}


/* writeInt8(buffer, value, offset) */
static int buffer_writeInt8 (lua_State *L) {
  BUFFER_UDATA(L)

  BUFFER_GETOFFSET(L, 3) /* sets index */

  *((int8_t*)(buffer + index - 1)) = luaL_checknumber(L, 2);
  return 0;
}

/* writeUInt16LE(buffer, value, offset) */
static int buffer_writeUInt16BE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 3, 2) /* sets index */

  *((uint16_t*)(buffer + index - 1)) = __bswap16( luaL_checknumber(L, 2) );
  return 0;
}

/* writeUInt16LE(buffer, value, offset) */
static int buffer_writeUInt16LE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 3, 2) /* sets index */

  *((uint16_t*)(buffer + index - 1)) = luaL_checknumber(L, 2);
  return 0;
}

/* writeUInt32LE(buffer, value, offset) */
static int buffer_writeUInt32BE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 3, 4) /* sets index */

  *((uint32_t*)(buffer + index - 1)) = __bswap32( luaL_checknumber(L, 2) );
  return 0;
}


/* writeUInt32LE(buffer, value, offset) */
static int buffer_writeUInt32LE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 3, 4) /* sets index */

  *((uint32_t*)(buffer + index - 1)) = luaL_checknumber(L, 2);
  return 0;
}

/* writeInt32LE(buffer, value, offset) */
static int buffer_writeInt32BE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 3, 4) /* sets index */

  *((int32_t*)(buffer + index - 1)) = __bswap32( luaL_checknumber(L, 2) );
  return 0;
}


/* writeInt32LE(buffer, write, offset) */
static int buffer_writeInt32LE (lua_State *L) {
  BUFFER_UDATA(L)

  _BUFFER_GETOFFSET(L, 3, 4) /* sets index */

  *((int32_t*)(buffer + index - 1)) = luaL_checknumber(L, 2);
  return 0;
}

/* writeHexLower(buffer, write, offset) */
static int buffer_write_hex_lower (lua_State *L) {
  size_t write_int;
  size_t out_int;
  char tmp[32];

  BUFFER_UDATA(L)

  write_int = luaL_checknumber(L, 2);
  out_int = snprintf(tmp, 32, "%zx", write_int);

  size_t index = (size_t)lua_tointeger(L, 3);
  if (index < 1 || index - 1 > buffer_len - out_int) {
    return luaL_argerror(L, 3, "Index out of bounds");
  }

  int i;
  for (i = 0;tmp[i];) { /* this should be faster than memcpy */
      *(buffer + index++ - 1) = tmp[i++];
  }

  lua_pushnumber(L, out_int);
  return 1;
}

/* writeHexUpper(buffer, write, offset) */
static int buffer_write_hex_upper (lua_State *L) {
  size_t write_int;
  size_t out_int;
  char tmp[32];

  BUFFER_UDATA(L)

  write_int = luaL_checknumber(L, 2);
  out_int = snprintf(tmp, 32, "%zX", write_int);

  size_t index = (size_t)lua_tointeger(L, 3);
  if (index < 1 || index - 1 > buffer_len - out_int) {
    return luaL_argerror(L, 3, "Index out of bounds");
  }

  int i;
  for (i = 0;tmp[i];) { /* this should be faster than memcpy */
      *(buffer + index++ - 1) = tmp[i++];
  }

  lua_pushnumber(L, out_int);
  return 1;
}

/* __gc(buffer) */
static int buffer__gc (lua_State *L) {
  MemSlice *ms = luaL_checkudata(L, 1, "lev.buffer");
  lev_slab_decRef( ms->mb );
  return 0;
}

/* __eq(buffer1, buffer2) */
static int buffer__eq (lua_State *L) {
  MemSlice *ms1 = luaL_checkudata(L, 1, "lev.buffer");
  MemSlice *ms2 = luaL_checkudata(L, 2, "lev.buffer");
  if (ms1->until != ms2->until) {
    lua_pushboolean(L, 0);
  } else {
    lua_pushboolean(L, memcmp(ms1->slice, ms2->slice, ms1->until) == 0 ? 1 : 0 );
  }
  return 1;
}

/* __len(buffer) */
static int buffer__len (lua_State *L) {
  MemSlice *ms = luaL_checkudata(L, 1, "lev.buffer");

  lua_pushnumber(L, ms->until);
  return 1;
}

/* __index(buffer, key) */
static int buffer__index (lua_State *L) {
  if (LUA_TNUMBER != lua_type(L, 2)) { /* key should be a number */
    const char *value = lua_tolstring(L, 2, NULL);
    if (value[0] == 'l' && 0 == strncmp(value, "length", 7)) { /* ghetto deprecation! */
      MemSlice *ms = luaL_checkudata(L, 1, "lev.buffer");
      lua_pushnumber(L, ms->until);
      return 1;
    }
    lua_getmetatable(L, 1); /*get userdata metatable */
    lua_pushvalue(L, 2);
    lua_rawget(L, -2); /*check environment table for field*/
    return 1;
  }

  BUFFER_UDATA(L)

  BUFFER_GETOFFSET(L, 2) /* sets index */

  /* skip first size_t length */
  lua_pushnumber(L, *((uint8_t*)(buffer + index - 1)));
  return 1;
}

int buffer__pairs_aux(lua_State *L) {
  MemSlice *ms = luaL_checkudata(L, -2, "lev.buffer");
  unsigned char *buffer = (unsigned char *)ms->slice;
  size_t buffer_len = ms->until;

  size_t index = luaL_checknumber(L,-1);
  if( index >= buffer_len ) {
    return 0;
  }
        
  lua_pushinteger(L, index+1 );
  lua_pushnumber(L, *((uint8_t*)(buffer + index)));
  return 2;
}


static int buffer__pairs (lua_State *L) {
  (void)lev_checkbufobj(L, -1);
  lua_pushcfunction(L, buffer__pairs_aux);
  lua_insert(L,-2);
  lua_pushinteger(L,0);
  return 3;
}

static int buffer__concat (lua_State *L) {
  /*
    Now that we are on on Slab/MemBlock foundation
    we should be able to do some cool things here like
    notice that we are on the same MemBlock and simply
    return a new MemSlice adjusted to cover the unioned
    area. Obviously, if we are not on the same MemBlock
    we will have to alloc a new MemBlock from lev_slab.
  */

  size_t new_size;
  MemSlice *new_ms;

  size_t first_buffer_len;
  unsigned char *first_buffer = NULL;
  MemSlice *first_ms = NULL;

  size_t second_buffer_len;
  unsigned char *second_buffer = NULL;
  MemSlice *second_ms = NULL;

  int first_type = lua_type(L, 1);
  switch (first_type) {
    case LUA_TUSERDATA:
      first_ms = luaL_checkudata(L, 1, "lev.buffer");
      first_buffer = (unsigned char *)first_ms->slice;
      first_buffer_len = first_ms->until;
      break;
    case LUA_TSTRING:
      first_buffer = (unsigned char *)lua_tolstring(L, 2, &first_buffer_len);
      break;
    default:
      return luaL_argerror(L, 1, "Must be of type 'String' or cBuffer");
      break;
  }

  int second_type = lua_type(L, 2);
  switch (second_type) {
    case LUA_TUSERDATA:
      second_ms = luaL_checkudata(L, 2, "lev.buffer");
      second_buffer = (unsigned char *)second_ms->slice;
      second_buffer_len = second_ms->until;
      break;
    case LUA_TSTRING:
      second_buffer = (unsigned char *)lua_tolstring(L, 2, &second_buffer_len);
      break;
    default:
      return luaL_argerror(L, 2, "Must be of type 'String' or cBuffer");
      break;
  }

  if (first_type == second_type
      && first_ms->mb == second_ms->mb
      && ((
          first_ms->slice > second_ms->slice
          && second_ms->slice + second_ms->until == first_ms->slice
         )
         || /* or other way around */
         (
          second_ms->slice > first_ms->slice
          && first_ms->slice + first_ms->until == second_ms->slice
         ))
      ) {

    /*
      cool, we're on the same MemBlock and back-to-back.
      we should be able to union both objs via a single MemSlice
    */

    lev_pushbuffer_from_mb(
         L
        ,first_ms->mb
        ,first_ms->until + second_ms->until
        ,(first_ms->slice > second_ms->slice ? second_ms->slice : first_ms->slice)
      ); /* automatically incRef's mb */

    /* userdata type is set automatically */
  } else { /* not on the same memblock */
    new_size = first_buffer_len + second_buffer_len;

    if (first_type == LUA_TUSERDATA
        && first_ms->mb->bytes + first_ms->mb->nbytes - first_ms->until == first_ms->slice
        && first_ms->mb->size - first_ms->mb->nbytes >= second_buffer_len
        ) {
      /*
        if we are bilateral but not back-to-back
        and if after first_ms's slice we have
        space for second_ms, simply copy second_ms
        to the same memblock.
      */
        
      lev_pushbuffer_from_mb(
           L
          ,first_ms->mb
          ,first_ms->until + second_buffer_len
          ,first_ms->slice
        ); /* automatically incRef's mb */
      first_ms->mb->nbytes += second_buffer_len;
      new_ms = first_ms;
    } else {
      new_ms = lev_buffer_new(L, new_size, (const char *)first_buffer, first_buffer_len);
    }

    /* copy other_buffer to new userdata */
    memcpy(
       new_ms->slice + first_buffer_len /* lev_resizebuffer copies only half of the data, so seek */
      ,second_buffer
      ,second_buffer_len
    );
  }
  return 1;
}

/* __newindex(buffer, key, value) */
static int buffer__newindex (lua_State *L) {
  int entry_type;
  size_t value_len;
  const char *value;
  MemSlice *value_ms;

  if (LUA_TNUMBER != lua_type(L, 2)) { /* key should be a number */
    return luaL_argerror(L, 1, "We only support setting indices by type Number");
  }

  BUFFER_UDATA(L)
  BUFFER_GETOFFSET(L, 2) /* sets index */
  
  entry_type = lua_type(L, 3);
  switch (entry_type) {
    case LUA_TUSERDATA:
      value_ms = luaL_checkudata(L, 3, "lev.buffer");
      lev_memslice_append_mem(
           ms
          ,index - 1 /* offset */
          ,(const char *)value_ms->slice
          ,value_ms->until
        );
      break;
    case LUA_TSTRING:
      value = lua_tolstring(L, 3, &value_len);
      lev_memslice_append_mem(
           ms
          ,index - 1 /* offset */
          ,(const char *)value
          ,value_len
        );
      break;
    case LUA_TNUMBER:
      value_len = (size_t)lua_tointeger(L, 3); /* using value_len here as value */
      if (value_len > 255) {
        return luaL_argerror(L, 3, "0 < Value < 255");
      }
      *((unsigned char *)(buffer + index - 1)) = value_len;
      break;
    default:
      return luaL_argerror(L, 3, "Must be of type 'String', 'Number' or 'cBuffer'");
      break;
  }

  return 0;
}

static int buffer_debug(lua_State *L) {
  MemSlice *ms = luaL_checkudata(L, 1, "lev.buffer");
  printf("[%p] |||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n", ms->slice);
  printf("[%p] BUFFER DEBUG(r=%d, p=%p)\n", ms, ms->mb->refcount, ms->mb->allocator);
  printf("[%p] |||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n", ms->slice);
  return 0;
}


/******************************************************************************/


static luaL_reg methods[] = {
   {"toString", buffer_tostring}
  ,{"upUntil", buffer_upuntil}
  ,{"inspect", buffer_inspect}
  ,{"fill", buffer_fill}
  ,{"find", buffer_find}
  ,{"slice", buffer_slice}

  /* convenience */
  ,{"s", buffer_tostring}
  ,{"debug", buffer_debug}

  /* binary helpers */
  ,{"readUInt8", buffer__index} /* same as __index */
  ,{"readInt8", buffer_readInt8}
  ,{"readUInt16BE", buffer_readUInt16BE}
  ,{"readUInt16LE", buffer_readUInt16LE}
  ,{"readUInt32BE", buffer_readUInt32BE}
  ,{"readUInt32LE", buffer_readUInt32LE}
  ,{"readInt32BE", buffer_readInt32BE}
  ,{"readInt32LE", buffer_readInt32LE}  
  ,{"writeUInt8", buffer_writeUInt8}
  ,{"writeInt8", buffer_writeInt8}
  ,{"writeUInt16BE", buffer_writeUInt16BE}
  ,{"writeUInt16LE", buffer_writeUInt16LE}
  ,{"writeUInt32BE", buffer_writeUInt32BE}
  ,{"writeUInt32LE", buffer_writeUInt32LE}
  ,{"writeInt32BE", buffer_writeInt32BE}
  ,{"writeInt32LE", buffer_writeInt32LE}

  ,{"writeHexLower", buffer_write_hex_lower}
  ,{"writeHexUpper", buffer_write_hex_upper}

   /* meta */
  ,{"__gc", buffer__gc}
  ,{"__eq", buffer__eq}
  ,{"__len", buffer__len}
  ,{"__index", buffer__index}
  ,{"__pairs", buffer__pairs}
  ,{"__concat", buffer__concat}
  ,{"__tostring", buffer_tostring}
  ,{"__newindex", buffer__newindex}
  ,{NULL, NULL}
};


static luaL_reg functions[] = {
  {"new", buffer_new}
  ,{"isBuffer", buffer_isbuffer}
  ,{ NULL, NULL }
};

void luaopen_lev_buffer(lua_State *L) {
  luaL_newmetatable(L, "lev.buffer");
  luaL_register(L, NULL, methods);
  lua_setfield(L, -1, "__index");

  lua_createtable(L, 0, ARRAY_SIZE(functions) - 1);
  luaL_register(L, NULL, functions);
  lua_setfield(L, -2, "buffer");
}