/*
 *  Copyright Kengo Nakajima
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

#include <stdio.h>
#include <math.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <arpa/inet.h>


#include "lua.h"
#include "lauxlib.h"

#define ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((x << 32) >> 32) )) << 32) | ntohl( ((uint32_t)(x >> 32)) ) )

#define htonll(x) ntohll(x)

#define MP_ENABLE_DEBUGPRINT 0

#if MP_ENABLE_DEBUGPRINT
#define MPDEBUGPRINT(...) fprintf( stderr, __VA_ARGS__ )
#else
#define MPDEBUGPRINT(...) 
#endif

typedef struct {
    unsigned char data[1024*1024];
    size_t used; /* start from zero */
    size_t capacity;
    int err;
} mpwbuf_t;

#define ERRORBIT_BUFNOLEFT 1
#define ERRORBIT_STRINGLEN 2
#define ERRORBIT_TYPE_LIGHTUSERDATA 4
#define ERRORBIT_TYPE_FUNCTION 8
#define ERRORBIT_TYPE_USERDATA 16
#define ERRORBIT_TYPE_THREAD 32
#define ERRORBIT_TYPE_UNKNOWN 64

typedef struct {
    MemBlock *mb;
    unsigned char *data; /* read buffer dont have to allocate buffer. */
    size_t ofs;
    size_t len;
    int err;
    int map_keying;
} mprbuf_t;

mpwbuf_t g_mpwbuf; /* single threaded! */

void mpwbuf_init(mpwbuf_t *b){
    b->used=0;
    b->capacity = sizeof(b->data);
    b->err=0;
}
void mprbuf_init(mprbuf_t *b, const unsigned char *p, size_t len ){
    b->mb = NULL;
    b->data = (unsigned char*)p;
    b->len = len;
    b->ofs=0;
    b->err=0;
    b->map_keying=0;
}

void mprbuf_mb_init(mprbuf_t *b, MemBlock *mb, const unsigned char *p, size_t len ){
    /* check to make sure that we are actually slicing the mb */
    assert(p >= mb->bytes && p <= (mb->bytes+mb->size) && mb->size >= len);
    b->mb = mb;
    b->data = (unsigned char*)p;
    b->len = len;
    b->ofs=0;
    b->err=0;
    b->map_keying=0;
}


size_t mpwbuf_left(mpwbuf_t *b){
    return b->capacity - b->used;
}
size_t mprbuf_left(mprbuf_t *b){
    return b->len - b->ofs;
}

void mp_rcopy( unsigned char *dest, unsigned char*from, size_t l ){
    size_t i;
    for(i=0;i<l;i++){
        dest[l-i-1]=from[i];
    }
}

size_t mpwbuf_append(mpwbuf_t *b, const unsigned char *toadd, size_t l){
    if(b->err){return 0;}
    if( mpwbuf_left(b)<l){
        b->err |= ERRORBIT_BUFNOLEFT;
        return 0;
    }
    memcpy( b->data + b->used, toadd,l);
    b->used += l;
    return l;
}
/*return packed size. 0 when error.*/
static size_t mpwbuf_pack_nil( mpwbuf_t *b ){
    unsigned char append[1]={ 0xc0 };    
    return mpwbuf_append(b,append,1);
}
static size_t mpwbuf_pack_boolean( mpwbuf_t *b, int i) {
    unsigned char append[1];
    if(i){
        append[0] = 0xc3;
    } else {
        append[0] = 0xc2;
    }
    return mpwbuf_append(b,append,1);
}
static size_t mpwbuf_pack_number( mpwbuf_t *b, lua_Number n ) {
    unsigned char buf[1+8];
    size_t len=0;

    if( isinf(n) ){
        buf[0] = 0xcb; /* double */
        if(n>0){
            buf[1] = 0x7f;
            buf[2] = 0xf0;
        } else {
            buf[1] = 0xff;
            buf[2] = 0xf0;
        }
        buf[3] = buf[4] = buf[5] = buf[6] = buf[7] = buf[8] = 0;
        len += 1+8;
    } else if( isnan(n) ) {
        buf[0] = 0xcb;
        buf[1] = 0xff;
        buf[2] = 0xf8;
        buf[3] = buf[4] = buf[5] = buf[6] = buf[7] = buf[8] = 0;
        len += 1+8;        
    } else if(floor(n)==n){
        long long lv = (long long)n;
        if(lv>=0){
            if(lv<128){
                buf[0]=(char)lv;
                len=1;
            } else if(lv<256){
                buf[0] = 0xcc;
                buf[1] = (char)lv;
                len=2;
            } else if(lv<65536){
                buf[0] = 0xcd;
                short v = htons((short)lv);
                memcpy(buf+1,&v,2);
                len=1+2;
            } else if(lv<4294967296LL){
                buf[0] = 0xce;
                long v = htonl((long)lv);
                memcpy(buf+1,&v,4);
                len=1+4;
            } else {
                buf[0] = 0xcf;
                long long v = htonll((long long)lv);
                memcpy(buf+1,&v,8);
                len=1+8;
            }
        } else {
            if(lv >= -32){
                buf[0] = 0xe0 | (char)lv;
                len=1;
            } else if( lv >= -128 ){
                buf[0] = 0xd0;
                buf[1] = lv;
                len=2;
            } else if( lv >= -32768 ){
                short v = htons(lv&0xffff);
                buf[0] = 0xd1;
                memcpy(buf+1, &v,2);
                len=1+2;
            } else if( lv >= -2147483648LL ){
                int v = htonl(lv&0xffffffff);
                buf[0] = 0xd2;
                memcpy(buf+1,&v,4);
                len=1+4;
            } else{
                long long v = htonll(lv);
                buf[0] = 0xd3;
                memcpy(buf+1,&v,8);
                len=1+8;
            }
        }
    } else { /* floating point! */
        assert(sizeof(double)==sizeof(n));
        buf[0] = 0xcb;
        mp_rcopy(buf+1,(unsigned char*)&n,sizeof(double)); /* endianness */
        len=1+8;
    }
    return mpwbuf_append(b,buf,len);
}
static size_t mpwbuf_pack_string( mpwbuf_t *b, const unsigned char *sval, size_t slen ) {
    unsigned char topbyte=0;
    size_t wl=0;
    if(slen<32){
        topbyte = 0xa0 | (char)slen;
        wl = mpwbuf_append(b, &topbyte, 1 );
        wl += mpwbuf_append(b, sval, slen);
    } else if(slen<65536){
        topbyte = 0xda;
        wl = mpwbuf_append(b,&topbyte,1);
        unsigned short l = htons(slen);
        wl += mpwbuf_append(b,(unsigned char*)(&l),2);
        wl += mpwbuf_append(b,sval,slen);
    } else if(slen<4294967296LL-1){ /* TODO: -1 for avoiding (condition is always true warning) */
        topbyte = 0xdb;
        wl = mpwbuf_append(b,&topbyte,1);
        unsigned int l = htonl(slen);
        wl += mpwbuf_append(b,(unsigned char*)(&l),4);
        wl += mpwbuf_append(b,sval,slen);
    } else {
        b->err |= ERRORBIT_STRINGLEN;
    }
    return wl;    
}

static size_t mpwbuf_pack_anytype( mpwbuf_t *b, lua_State *L, int index ) ;


/*
from mplua:  void packTable(Packer& pk, int index) const {
check if this is an array
NOTE: This code strongly depends on the internal implementation
of Lua5.1. The table in Lua5.1 consists of two parts: the array part
and the hash part. The array part is placed before the hash part.
Therefore, it is possible to obtain the first key of the hash part
by using the return value of lua_objlen as the argument of lua_next.
If lua_next return 0, it means the table does not have the hash part,
that is, the table is an array.

Due to the specification of Lua, the table with non-continous integral
keys is detected as a table, not an array.
*/

/* index: index in stack */
static size_t mpwbuf_pack_table( mpwbuf_t *b, lua_State *L, int index ) {
    size_t nstack = lua_gettop(L);
    size_t l = lua_objlen(L,index);
    
    size_t wl=0;
    /*    fprintf(stderr, "mpwbuf_pack_table: lua_objlen: array part len: %d index:%d\n", (int)l,index);*/
    
    /*try array first, and then map.*/
    if(l>0){
        unsigned char topbyte;
        /* array!(ignore map part.) 0x90|n , 0xdc+2byte, 0xdd+4byte */
        if(l<16){
            topbyte = 0x90 | (unsigned char)l;
            wl = mpwbuf_append(b,&topbyte,1);
        } else if( l<65536){
            topbyte = 0xdc;
            wl = mpwbuf_append(b,&topbyte,1);
            unsigned short elemnum = htons(l);
            wl += mpwbuf_append(b,(unsigned char*)&elemnum,2);
        } else if( l<4294967296LL-1){ /* TODO: avoid C warn */
            topbyte = 0xdd;
            wl = mpwbuf_append(b,&topbyte,1);
            unsigned int elemnum = htonl(l);
            wl += mpwbuf_append(b,(unsigned char*)&elemnum,4);
        }
        
        int i;
        for(i=1;i<=(int)l;i++){
            lua_rawgeti(L,index,i); /* push table value to stack */
            wl += mpwbuf_pack_anytype(b,L,nstack+1);
            lua_pop(L,1); /* repair stack */
        }
    } else {
        /* map! */
        l=0;
        lua_pushnil(L);
        while(lua_next(L,index)){
            l++;
            lua_pop(L,1);
        }
        /* map fixmap, 16,32 : 0x80|num, 0xde+2byte, 0xdf+4byte */
        unsigned char topbyte=0;
        if(l<16){
            topbyte = 0x80 | (char)l;
            wl = mpwbuf_append(b,&topbyte,1);
        }else if(l<65536){
            topbyte = 0xde;
            wl = mpwbuf_append(b,&topbyte,1);
            unsigned short elemnum = htons(l);
            wl += mpwbuf_append(b,(unsigned char*)&elemnum,2);
        }else if(l<4294967296LL-1){
            topbyte = 0xdf;
            wl = mpwbuf_append(b,&topbyte,1);
            unsigned int elemnum = htonl(l);
            wl += mpwbuf_append(b,(unsigned char*)&elemnum,4);
        }
        lua_pushnil(L); /* nil for first iteration on lua_next */
        while( lua_next(L,index)){
            wl += mpwbuf_pack_anytype(b,L,nstack+1); /* -2:key */
            wl += mpwbuf_pack_anytype(b,L,nstack+2); /* -1:value */
            lua_pop(L,1); /* remove value and keep key for next iteration */
        }
    }
    return wl;
}

static size_t mpwbuf_pack_anytype( mpwbuf_t *b, lua_State *L, int index ) {
/*
        int top = lua_gettop(L); 
        fprintf(stderr, "mpwbuf_pack_anytype: top:%d index:%d\n", top, index);
*/
    int t = lua_type(L,index);
    switch(t){
    case LUA_TNIL:
        return mpwbuf_pack_nil(&g_mpwbuf);
    case LUA_TBOOLEAN:
        {
            int iv = lua_toboolean(L,index);
            return mpwbuf_pack_boolean(&g_mpwbuf,iv);
        }
    case LUA_TNUMBER:
        {
            lua_Number nv = lua_tonumber(L,index);
            return mpwbuf_pack_number(&g_mpwbuf,nv);
        }
        break;
    case LUA_TSTRING:
        {
            size_t slen;
            const char *sval = luaL_checklstring(L,index,&slen);
            return mpwbuf_pack_string(&g_mpwbuf,(const unsigned char*)sval,slen);
        }
        break;
    case LUA_TTABLE:
        return mpwbuf_pack_table(&g_mpwbuf, L, index );
    case LUA_TLIGHTUSERDATA:
        b->err |= ERRORBIT_TYPE_LIGHTUSERDATA;
        break;
    case LUA_TFUNCTION:
        b->err |= ERRORBIT_TYPE_FUNCTION;
        break;
    case LUA_TUSERDATA:
        {
          if (luaL_getmetafield(L, index, "readUInt8")) {
            lua_pop(L,1);
            MemSlice *ms = luaL_checkudata(L, index, "lev.buffer");
            return mpwbuf_pack_string(&g_mpwbuf,(const unsigned char*)ms->slice, ms->until);
          }
        }
        b->err |= ERRORBIT_TYPE_USERDATA;
        break;
    case LUA_TTHREAD:
        b->err |= ERRORBIT_TYPE_THREAD;
        break;
    default:
        b->err |= ERRORBIT_TYPE_UNKNOWN;
        break;
    }

    return 0;    
}

/* return num of return value */
static int msgpack_pack_api( lua_State *L ) {
    MemBlock *out_mb;
    mpwbuf_init( &g_mpwbuf);

    size_t wlen = mpwbuf_pack_anytype(&g_mpwbuf,L,1);
    if(wlen>0 && g_mpwbuf.err == 0 ) {
      out_mb = lev_slab_getBlock( g_mpwbuf.used );
      lev_pushbuffer_from_mb(
         L
        ,out_mb
        ,g_mpwbuf.used
        ,out_mb->bytes + out_mb->nbytes
      ); /* automatically incRef's mb */

      memcpy(
        out_mb->bytes + out_mb->nbytes
        ,g_mpwbuf.data
        ,g_mpwbuf.used
        );

      return 1;
    } else {
        const char *errmsg = "unknown error";
        if( g_mpwbuf.err & ERRORBIT_BUFNOLEFT ){
            errmsg = "no buffer left";
        } else if ( g_mpwbuf.err &  ERRORBIT_STRINGLEN ){
            errmsg = "string too long";
        } else if ( g_mpwbuf.err & ERRORBIT_TYPE_LIGHTUSERDATA ){
            errmsg = "invalid type: lightuserdata";
        } else if ( g_mpwbuf.err & ERRORBIT_TYPE_FUNCTION ){
            errmsg = "invalid type: function";
        } else if ( g_mpwbuf.err & ERRORBIT_TYPE_USERDATA ){
            errmsg = "invalid type: userdata";
        } else if ( g_mpwbuf.err & ERRORBIT_TYPE_THREAD ){
            errmsg = "invalid type: thread";
        } else if ( g_mpwbuf.err & ERRORBIT_TYPE_UNKNOWN ){
            errmsg = "invalid type: unknown";
        }

        lua_pushfstring( L, errmsg );
        lua_error(L);
        return 2;
    }
}

/* push a table */
static void mprbuf_unpack_anytype( mprbuf_t *b, lua_State *L );
static void mprbuf_unpack_array( mprbuf_t *b, lua_State *L, int arylen ) {
/*        lua_newtable(L);*/
    lua_createtable(L,arylen,0);
    int i;
    for(i=0;i<arylen;i++){
        mprbuf_unpack_anytype(b,L); /* array element */
        if(b->err)break;
        lua_rawseti(L, -2, i+1);
    }
}
static void mprbuf_unpack_map( mprbuf_t *b, lua_State *L, int maplen ) {
    /* return a table */
    /*    lua_newtable(L); // push {}*/
    lua_createtable(L,0,maplen);
    int i;
    for(i=0;i<maplen;i++){
        b->map_keying = 1;
        mprbuf_unpack_anytype(b,L); /*key*/
        b->map_keying = 0;
        mprbuf_unpack_anytype(b,L); /*value*/
        lua_rawset(L,-3);
    }
}

static void mprbuf_unpack_anytype( mprbuf_t *b, lua_State *L ) {
    if( mprbuf_left(b) < 1){
        b->err |= 1;
        return;
    }
    unsigned char t = b->data[ b->ofs ];
    /*        fprintf( stderr, "mprbuf_unpack_anytype: topbyte:%x ofs:%d len:%d\n",(int)t, (int)b->ofs, (int)b->len );*/

    b->ofs += 1; /* for toptypebyte */
    
    if(t<0x80){ /* fixed num */
        lua_pushnumber(L,(lua_Number)t);
        return;
    }

    unsigned char *s = b->data + b->ofs;
    
    if(t>=0x80 && t <=0x8f){ /* fixed map */
        size_t maplen = t & 0xf;
        mprbuf_unpack_map(b,L,maplen);
        return;
    }
    if(t>=0x90 && t <=0x9f){ /* fixed array */
        size_t arylen = t & 0xf;
        mprbuf_unpack_array(b,L,arylen);
        return;
    }

    if(t>=0xa0 && t<=0xbf){ /* fixed string */
        size_t slen = t & 0x1f;
        if( mprbuf_left(b) < slen ){
            b->err |= 1;
            return;
        }
        if (b->map_keying || !b->mb) {
          lua_pushlstring(L,(const char*)s,slen);
        } else {
          lev_pushbuffer_from_mb(
             L
            ,b->mb
            ,slen
            ,s
          ); /* automatically incRef's mb */
        }
        b->ofs += slen;
        return;
    }
    if(t>0xdf){ /* fixnum_neg (-32 ~ -1) */
        unsigned char ut = t;
        lua_Number n = ( 256 - ut ) * -1;
        lua_pushnumber(L,n);
        return;
    }
    
    switch(t){
    case 0xc0: /* nil */
        lua_pushnil(L);
        return;
    case 0xc2: /* false */
        lua_pushboolean(L,0);
        return;
    case 0xc3: /* true */
        lua_pushboolean(L,1);
        return;

    case 0xca: /* float */
        if(mprbuf_left(b)>=4){
            float f;
            mp_rcopy( (unsigned char*)(&f), s,4); /* endianness */
            lua_pushnumber(L,f);
            b->ofs += 4;
            return;
        }
        break;
    case 0xcb: /* double */
        if(mprbuf_left(b)>=8){
            double v;
            mp_rcopy( (unsigned char*)(&v), s,8); /* endianness */
            lua_pushnumber(L,v);
            b->ofs += 8;
            return;
        }
        break;
        
    case 0xcc: /* 8bit large posi int */
        if(mprbuf_left(b)>=1){
            lua_pushnumber(L,(unsigned char) s[0] );
            b->ofs += 1;
            return;
        }
        break;
    case 0xcd: /* 16bit posi int */
        if(mprbuf_left(b)>=2){
            unsigned short v = ntohs( *(short*)(s) );
            lua_pushnumber(L,v);
            b->ofs += 2;
            return;
        }
        break;
    case 0xce: /* 32bit posi int */
        if(mprbuf_left(b)>=4){
            unsigned long v = ntohl( *(long*)(s) );
            lua_pushnumber(L,v);
            b->ofs += 4;
            return;
        }
        break;
    case 0xcf: /* 64bit posi int */
        if(mprbuf_left(b)>=8){
            unsigned long long v = ntohll( *(long long*)(s));
            lua_pushnumber(L,v);
            b->ofs += 8;
            return;
        }
        break;
    case 0xd0: /* 8bit neg int */
        if(mprbuf_left(b)>=1){
            lua_pushnumber(L, (signed char) s[0] );
            b->ofs += 1;
            return;
        }
        break;
    case 0xd1: /* 16bit neg int */
        if(mprbuf_left(b)>=2){
            short v = *(short*)(s);
            v = ntohs(v);
            lua_pushnumber(L,v);
            b->ofs += 2;
            return;
        }
        break;
    case 0xd2: /* 32bit neg int */
        if(mprbuf_left(b)>=4){
            int v = *(long*)(s);
            v = ntohl(v);
            lua_pushnumber(L,v);
            b->ofs += 4;
            return;
        }
        break;
    case 0xd3: /* 64bit neg int */
        if(mprbuf_left(b)>=8){
            long long v = *(long long*)(s);
            v = ntohll(v);
            lua_pushnumber(L,v);
            b->ofs += 8;
            return;
        }
        break;
    case 0xda: /* long string len<65536 */
        if(mprbuf_left(b)>=2){
            size_t slen = ntohs(*((unsigned short*)(s)));
            b->ofs += 2;
            if(mprbuf_left(b)>=slen){
              if (b->map_keying || !b->mb) {
                lua_pushlstring(L,(const char*)b->data+b->ofs,slen);
              } else {
                lev_pushbuffer_from_mb(
                   L
                  ,b->mb
                  ,slen
                  ,b->data+b->ofs
                ); /* automatically incRef's mb */
              }
              b->ofs += slen;
              return;
            }
        }
        break;
    case 0xdb: /* longer string */
        if(mprbuf_left(b)>=4){
            size_t slen = ntohl(*((unsigned int*)(s)));
            b->ofs += 4;
            if(mprbuf_left(b)>=slen){
              if (b->map_keying || !b->mb) {
                lua_pushlstring(L,(const char*)b->data+b->ofs,slen);
              } else {
                lev_pushbuffer_from_mb(
                   L
                  ,b->mb
                  ,slen
                  ,b->data+b->ofs
                ); /* automatically incRef's mb */
              }
              b->ofs += slen;
              return;
            }
        }

        break;

    case 0xdc: /* ary16 */
        if(mprbuf_left(b)>=2){
            unsigned short elemnum = ntohs( *((unsigned short*)(b->data+b->ofs) ) );
            b->ofs += 2;
            mprbuf_unpack_array(b,L,elemnum);
            return;
        }
        break;
    case 0xdd: /* ary32 */
        if(mprbuf_left(b)>=4){
            unsigned int elemnum = ntohl( *((unsigned int*)(b->data+b->ofs)));
            b->ofs += 4;
            mprbuf_unpack_array(b,L,elemnum);
            return;
        }
        break;
    case 0xde: /* map16 */
        if(mprbuf_left(b)>=2){
            unsigned short elemnum = ntohs( *((unsigned short*)(b->data+b->ofs)));
            b->ofs += 2;
            mprbuf_unpack_map(b,L,elemnum);
            return;
        }
        break;
    case 0xdf: /* map32 */
        if(mprbuf_left(b)>=4){
            unsigned int elemnum = ntohl( *((unsigned int*)(b->data+b->ofs)));
            b->ofs += 4;
            mprbuf_unpack_map(b,L,elemnum);
            return;
        }
        break;
    default:
        break;
    }
    b->err |= 1;
}


static int msgpack_unpack_api( lua_State *L ) {
    size_t len;
    const char *s;
    MemSlice *ms = luaL_checkudata(L, 1, "lev.buffer");
    len = ms->until;
    s = (const char *)ms->slice;

    if(!s){
        lua_pushstring(L,"arg must be a buffer");
        lua_error(L);
        return 2;
    }
    if(len==0){
        lua_pushnil(L);
        return 1;
    }

    mprbuf_t rb;
    mprbuf_mb_init(&rb, ms->mb, (const unsigned char*)s,len);

    lua_pushnumber(L,-123456); /* push readlen and replace it later */
    mprbuf_unpack_anytype(&rb,L);

    /*    fprintf(stderr, "mprbuf_unpack_anytype: ofs:%d len:%d err:%d\n", (int)rb.ofs, (int)rb.len, rb.err );*/
    
    if( rb.ofs >0 && rb.err==0){
        lua_pushnumber(L,rb.ofs);
        lua_replace(L,-3); /* replace dummy len */
        /*        fprintf(stderr, "msgpack_unpack_api: unpacked len: %d\n", (int)rb.ofs );*/
        return 2;
    } else{
        /*        lua_pushfstring(L,"msgpack_unpack_api: unsupported type or buffer short. error code: %d\n", rb.err );*/
        /*        lua_error(L);*/
        lua_pushnil(L);
        lua_replace(L,-3);
        lua_pushnil(L);
        lua_replace(L,-2);        
        return 2;        
    }
}        

typedef enum {
    /* containers without size bytes */
    MPCT_FIXARRAY,
    MPCT_FIXMAP,
    /* containers with size bytes */
    MPCT_ARRAY16,
    MPCT_MAP16,
    MPCT_ARRAY32,
    MPCT_MAP32,
    /* direct values with size bytes */
    MPCT_RAW16,
    MPCT_RAW32,    
    /* direct values without size bytes */
    MPCT_RAW,
    MPCT_FLOAT,
    MPCT_DOUBLE,
    MPCT_UINT8,
    MPCT_UINT16,
    MPCT_UINT32,
    MPCT_UINT64,
    MPCT_INT8,
    MPCT_INT16,
    MPCT_INT32,
    MPCT_INT64,


    
} MP_CONTAINER_TYPE;

int MP_CONTAINER_TYPE_is_container( MP_CONTAINER_TYPE t ) {
    if( t >= MPCT_FIXARRAY && t <= MPCT_MAP32 ) {
        return 1;
    } else {
        return 0;
    }
}

char *MP_CONTAINER_TYPE_to_s( MP_CONTAINER_TYPE t ) {
    switch(t){
    case MPCT_FIXARRAY: return "fixary";
    case MPCT_FIXMAP: return "fixmap";
    case MPCT_RAW: return "fixraw";
    case MPCT_FLOAT: return "float";
    case MPCT_DOUBLE: return "double";
    case MPCT_UINT8: return "u8";
    case MPCT_UINT16: return "u16";
    case MPCT_UINT32: return "u32";
    case MPCT_UINT64: return "u64";
    case MPCT_INT8: return "i8";
    case MPCT_INT16: return "i16";
    case MPCT_INT32: return "i32";
    case MPCT_INT64: return "i64";
    case MPCT_ARRAY16: return "ary16";
    case MPCT_MAP16: return "map16";
    case MPCT_RAW16: return "raw16";
    case MPCT_ARRAY32: return "ary32";
    case MPCT_MAP32: return "map32";
    case MPCT_RAW32: return "raw32";        
    default:
        assert( !"not impl");
    }
}
int MP_CONTAINER_TYPE_sizesize( MP_CONTAINER_TYPE t) {
    if( t == MPCT_ARRAY16 || t == MPCT_MAP16 || t == MPCT_RAW16 ){
        return 2;
    } else if( t == MPCT_ARRAY32 || t == MPCT_MAP32 || t == MPCT_RAW32 ){
        return 4;
    } else {
        return 0;
    }
}
int MP_CONTAINER_TYPE_is_map( MP_CONTAINER_TYPE t) {
    if( t == MPCT_FIXMAP || t == MPCT_MAP16 || t == MPCT_MAP32 ) return 1; else return 0;
}

typedef struct {
    MP_CONTAINER_TYPE t;
    size_t expect;
    size_t sofar;

    /* for containers with size bytes */
    char sizebytes[4];
    size_t sizesize; /* array16,map16,raw16:2 array32,map32,raw32:4 */
    size_t sizesofar;
} mpstackent_t;

typedef struct {
    char *buf;
    size_t size;
    size_t used;
    
    mpstackent_t stack[256];
    size_t nstacked;
    size_t resultnum;
} unpacker_t;

void unpacker_init( unpacker_t *u, size_t maxsize ) {
    u->nstacked = 0;
    u->resultnum = 0;
    u->buf = (char*) malloc( maxsize );
    assert(u->buf);
    u->used = 0;
    u->size = maxsize;
}
#define elementof(x) ( sizeof(x) / sizeof(x[0]))
void mpstackent_init( mpstackent_t *e, MP_CONTAINER_TYPE t, size_t expect ) {
    e->t = t;
    e->expect = expect;
    e->sofar = 0;
    e->sizesize = MP_CONTAINER_TYPE_sizesize(t);
    e->sizesofar = 0;
}



/* move 1 byte forward */
int unpacker_progress( unpacker_t *u, char ch ) {
    int i;
    for(i = u->nstacked-1; i >= 0; i-- ) {
        mpstackent_t *e = & u->stack[ i ];
        if( e->sizesofar < e->sizesize ) {
            MPDEBUGPRINT( "(%s Size:%d/%d)", MP_CONTAINER_TYPE_to_s(e->t), (int)e->sizesofar, (int)e->sizesize );
            e->sizebytes[ e->sizesofar ] = ch;
            e->sizesofar ++;
            if(e->sizesofar == e->sizesize){
                if( e->sizesize == 2 ){
                    e->expect = ntohs( *(short*)( e->sizebytes ) );
                    if( MP_CONTAINER_TYPE_is_map(e->t) ) e->expect *= 2;
                    MPDEBUGPRINT( "expect:%d ", (int)e->expect );
                } else if(e->sizesize == 4 ){
                    e->expect = ntohl( *(int*)( e->sizebytes ) );
                    if( MP_CONTAINER_TYPE_is_map(e->t) ) e->expect *= 2;                    
                    MPDEBUGPRINT( "expect:%d ", (int)e->expect );                    
                } else {
                    assert(!"possible bug in msgpack, not parse error" );
                }
            }
            break;
        } 
        e->sofar++;
        MPDEBUGPRINT( "(%s %d/%d)", MP_CONTAINER_TYPE_to_s(e->t),(int)e->sofar,(int)e->expect) ;
        /*        assert( e->sofar <= e->expect );*/
        if(e->sofar < e->expect){
            break;
        }
        u->nstacked --;
        assert(u->nstacked >= 0);
        MPDEBUGPRINT( "fill-pop! " );
    }
    MPDEBUGPRINT( "\n");
    if( u->nstacked == 0 ) {
        MPDEBUGPRINT( "got result!\n" );
        u->resultnum ++;
        return 1;
    }
    return 0;
}
mpstackent_t *unpacker_top( unpacker_t *u ) {
    if( u->nstacked == 0 ) {
        return NULL;
    } else {
        return & u->stack[ u->nstacked - 1 ];
    }
}


void unpacker_progress_datasize( unpacker_t *u, size_t progress) {
    MPDEBUGPRINT( ">%d>",(int)progress);
    mpstackent_t *top = unpacker_top(u);
    size_t need = top->expect - top->sofar;
    assert( progress <= need );
    top->sofar += progress; 
    if( top->sofar == top->expect ){
        top->sofar -= 1; /* sofar++ is in unpacker_progress! */
        MPDEBUGPRINT("[DS]");
        unpacker_progress(u,0);
    }
}


/* error:-1 */
/* give expect=0 when need size bytes. */
int unpacker_push( unpacker_t *u, MP_CONTAINER_TYPE t, int expect ) {
    MPDEBUGPRINT( "push: t:%s expect:%d\n", MP_CONTAINER_TYPE_to_s(t), expect );
    if( u->nstacked >= elementof(u->stack)) return -1;
    mpstackent_t *ent = & u->stack[ u->nstacked ];
    mpstackent_init( ent, t, expect );
    u->nstacked ++;
    /* handle empty container */
    if( MP_CONTAINER_TYPE_sizesize(t)==0 && expect == 0 ){
        MPDEBUGPRINT("[PUSH]");        
        unpacker_progress(u, 0x0);
    }
    return 0;
}


/* need bytes, not values */
int unpacker_container_needs_bytes( unpacker_t *u, size_t *dataneed ) {
    mpstackent_t *top = unpacker_top(u);
    if(!top)return 0;
    if( top->sizesofar < top->sizesize ) {
        *dataneed = 0;
        return 1;
    }
    
    if( top->sofar < top->expect ) {
        if( MP_CONTAINER_TYPE_is_container(top->t) ){
            return 0;
        } else {
            *dataneed = top->expect - top->sofar;
            return 1;
        }
    }
    return 0;
}




void unpacker_dump( unpacker_t *u ) {
    fprintf(stderr, "\n------\nnstacked:%d resultnum:%d\n", (int) u->nstacked, (int) u->resultnum );
    int i;
    for(i=0;i<u->nstacked;i++){
        fprintf(stderr, "  stack[%d]: %s %d/%d\n", i, MP_CONTAINER_TYPE_to_s(u->stack[i].t), (int)u->stack[i].sofar, (int)u->stack[i].expect );
    }
    fprintf(stderr, "--------\n");
}
                   

                  
int unpacker_feed( unpacker_t *u, char *p, size_t len ) {

    if(u->used + len > u->size ){
        return -1;
    }
    memcpy( u->buf + u->used, p, len );
    u->used += len;


    
    size_t i;
    for(i=0;i<len;i++){
        unsigned char ch = (unsigned char)( p[i] );

#if MP_ENABLE_DEBUGPRINT
        int k;
        for(k=0;k<u->nstacked;k++){ MPDEBUGPRINT(" "); }
#endif        
        MPDEBUGPRINT( "[%x]:", ch );

        size_t dataneed; /* for data, not for sizebytes. */
        if( unpacker_container_needs_bytes(u, &dataneed ) ){
            if(dataneed>0){
                size_t progress = len-i;
                if( progress > dataneed ) {
                    progress = dataneed;
                }
                MPDEBUGPRINT( "dneed:%d progress:%d len:%d ", (int) dataneed,(int)progress, (int)len );
                i += progress -1; /* sofar++ in unpacker_progress! */
                unpacker_progress_datasize( u, progress );
            } else {
                MPDEBUGPRINT("[FEED]");
                unpacker_progress(u,ch);
            }
            continue;
        }
            
        if( ch <= 0x7f ){ /* posfixnum */
            unpacker_progress(u,ch);
        } else if( ch>=0x80 && ch<=0x8f ){ /* fixmap */
            int n = ch & 0xf;
            if(unpacker_push( u, MPCT_FIXMAP, n*2 )<0) return -1;
        } else if( ch>=0x90 && ch<=0x9f ){ /* fixarray */
            int n = ch & 0xf;
            if(unpacker_push( u, MPCT_FIXARRAY, n )<0) return -1;
        } else if( ch>=0xa0 && ch<=0xbf ){ /* fixraw */
            int n = ch & 0x1f;
            if(unpacker_push( u, MPCT_RAW, n )<0) return -1;
        } else if( ch==0xc0){ /* nil */
            unpacker_progress(u,ch);            
        } else if( ch==0xc1){ /* reserved */
            return -1;
        } else if( ch==0xc2){ /* false */
            unpacker_progress(u,ch);            
        } else if( ch==0xc3){ /* true */
            unpacker_progress(u,ch);            
        } else if( ch>=0xc4 && ch<=0xc9){ /* reserved */
            return -1;
        } else if( ch==0xca){ /* float (4byte) */
            if(unpacker_push( u, MPCT_FLOAT, 4 )<0) return -1;
        } else if( ch==0xcb){ /* double (8byte) */
            if(unpacker_push( u, MPCT_DOUBLE, 8 )<0) return -1;
        } else if( ch==0xcc){ /* uint8 (1byte) */
            if(unpacker_push( u, MPCT_UINT8, 1 )<0) return -1;
        } else if( ch==0xcd){ /* uint16 (2byte) */
            if(unpacker_push( u, MPCT_UINT16, 2 )<0) return -1;
        } else if( ch ==0xce){ /* uint32 */
            if(unpacker_push( u, MPCT_UINT32, 4 )<0) return -1;
        } else if( ch ==0xcf){ /* uint64 */
            if(unpacker_push( u, MPCT_UINT64, 8 )<0) return -1;
        } else if( ch ==0xd0){ /* int8 */
            if(unpacker_push( u, MPCT_INT8, 1 )<0) return -1;
        } else if( ch == 0xd1){ /* int16 */
            if(unpacker_push( u, MPCT_INT16, 2 )<0) return -1;
        } else if( ch == 0xd2){ /* int32 */
            if(unpacker_push( u, MPCT_INT32, 4 )<0) return -1;            
        } else if( ch == 0xd3){ /* int64 */
            if(unpacker_push( u, MPCT_INT64, 8 )<0) return -1;
        } else if( ch >= 0xd4 && ch <= 0xd9 ) { /* reserved */
            return -1;
        } else if( ch == 0xda){ /* raw 16 */
            if(unpacker_push( u, MPCT_RAW16, 0 )<0) return -1;            
        } else if( ch == 0xdb){ /* raw 32 */
            if(unpacker_push( u, MPCT_RAW32, 0 )<0) return -1;            
        } else if( ch == 0xdc){ /* array 16 */
            if(unpacker_push( u, MPCT_ARRAY16, 0)<0) return -1;
        } else if( ch == 0xdd){ /* array 32 */
            if(unpacker_push( u, MPCT_ARRAY32, 0)<0) return -1;
        } else if( ch == 0xde){ /* map16 */
            if(unpacker_push( u, MPCT_MAP16, 0)<0) return -1;
        } else if( ch == 0xdf){ /* map32 */
            if(unpacker_push( u, MPCT_MAP32, 0)<0) return -1;
        } else if( ch >= 0xe0 ) { /* neg fixnum */
            unpacker_progress(u,ch);
        }
            
    }
    return 0;                
}
void unpacker_shift( unpacker_t *u, size_t l ) {
    assert( l <= u->used );
    memmove( u->buf, u->buf + l, u->used - l );
    u->used -= l;
}

static int msgpack_unpacker_feed_api( lua_State *L ) {
    unpacker_t *u =  luaL_checkudata( L, 1, "lev.mpack_unpacker" );
    size_t slen;
    const char *sval = luaL_checklstring(L, 2, &slen );
    MPDEBUGPRINT( "feed. used:%d len:%d\n",(int)u->used, (int)slen );
    int res = unpacker_feed( u, (char*)sval, slen );
    lua_pushnumber(L,res);
    return 1;
}
static int msgpack_unpacker_pull_api( lua_State *L ) {
    unpacker_t *u =  luaL_checkudata( L, 1, "lev.mpack_unpacker" );
    MPDEBUGPRINT( "pull:%d\n", (int)u->resultnum );
    if( u->resultnum == 0 ) {
        lua_pushnil(L);
    } else {
        mprbuf_t rb;
        mprbuf_init( &rb, (const unsigned char*) u->buf, u->used );
        mprbuf_unpack_anytype(&rb,L);
        unpacker_shift( u, rb.ofs );
        u->resultnum --;
    }
    return 1;
}
static int msgpack_unpacker_gc_api( lua_State *L ) {
    unpacker_t *u =  luaL_checkudata( L, 1, "lev.mpack_unpacker" );
    free(u->buf);
    u->buf = NULL;
    return 0;
}
    
static int msgpack_createUnpacker_api( lua_State *L ) {
    size_t bufsz = luaL_checknumber(L,1);
#if 0    
    /*    { aho=7, hoge = { 5,6,"7", {8,9,10} }, fuga="11" }*/
    char data[28] = { 0x83, 0xa3, 0x61, 0x68,
                      0x6f, 0x7, 0xa4, 0x66,
                      0x75, 0x67, 0x61, 0xa2,
                      0x31, 0x31, 0xa4, 0x68,
                      0x6f, 0x67, 0x65, 0x94,
                      0x5, 0x6, 0xa1, 0x37,
                      0x93, 0x8, 0x9, 0xa };
#endif    
    unpacker_t *u = (unpacker_t*) lua_newuserdata( L, sizeof(unpacker_t));

    unpacker_init( u, bufsz );
    luaL_getmetatable(L, "lev.mpack_unpacker" );
    lua_setmetatable(L, -2 );
    return 1;
}


static int msgpack_largetbl( lua_State *L ) {
    int n = luaL_checkint(L,1);
    lua_createtable(L,n,0);
    int i;
    for(i=0;i<n;i++){
        lua_pushnumber(L,i);
        lua_rawseti(L,-2,i+1);
    }
    return 1;
}


static const luaL_reg msgpack_f[] = {
     {"pack", msgpack_pack_api }
    ,{"unpack", msgpack_unpack_api }
    ,{"createUnpacker", msgpack_createUnpacker_api }
    ,{"largetbl", msgpack_largetbl }
    ,{"feed", msgpack_unpacker_feed_api }
    ,{"pull", msgpack_unpacker_pull_api }
    ,{NULL,NULL}
};

static const luaL_reg msgpack_unpacker_m[] = {
    {"feed", msgpack_unpacker_feed_api }
    ,{"pull", msgpack_unpacker_pull_api }
    ,{"__gc", msgpack_unpacker_gc_api }
    ,{NULL,NULL}
};


void luaopen_lev_mpack( lua_State *L ) {

  luaL_newmetatable(L, "lev.mpack_unpacker");
  luaL_register(L, NULL, msgpack_unpacker_m);
  lua_setfield(L, -1, "__index");

  lua_createtable(L, 0, ARRAY_SIZE(msgpack_f) - 1);
  luaL_register(L, NULL, msgpack_f);
  lua_setfield(L, -2, "mpack");
}
