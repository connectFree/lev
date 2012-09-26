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

#ifndef _LEV_SLAB_H_
#define _LEV_SLAB_H_

#include "lua.h"
#include "uv.h"

#define SLAB_MAXFREELIST 1024 * 16 * 2

/* --[  LEVSTRUCT_REF_ ]-- */
/* == refCount == */
/* a count of all pending request to know strength */
/* == threadref == */
/* if handle is created in a coroutine(not main thread), threadref is
   the reference to the coroutine in the Lua registery. 
   we release the reference when handle closed. 
   if handle is created in the main thread, threadref is LUA_NOREF.
   we must hold the coroutine, because in some cases(see issue #319) that the coroutine 
   referenced by nothing and would collected by gc, then uv's callback touch an 
   invalid pointer. */
/* == ref == */
/* ref is null when refCount is 0 meaning we're weak */
#define LEVBASE_REF_FIELDS  \
  lua_State *_L;            \
  int threadref;            \
  int refCount;             \
  int ref;                  \


typedef struct _lev_slab_allocator lev_slab_allocator_t;
typedef struct _MemBlock MemBlock;

struct _MemBlock {
  lev_slab_allocator_t *allocator;
  int refcount;  /* Number of references to this block */
  size_t size;   /* Size of the datablock */
  size_t nbytes; /* Number of bytes actually in buffer */
  unsigned char bytes[0];
};

struct _lev_slab_allocator {
  MemBlock *pool[SLAB_MAXFREELIST];
  int pool_count;
  int pool_min;
};

typedef struct _MemSlice {
  LEVBASE_REF_FIELDS
  MemBlock *mb;         /* our MemBlock */
  unsigned char *slice; /* begining of slice */
  size_t until;         /* range of how far we have sliced */
} MemSlice;

void lev_slab_fill();
MemBlock *lev_slab_getBlock(size_t size);
int lev_slab_incRef(MemBlock *block);
int lev_slab_decRefCount(MemBlock *block, int count);
#define lev_slab_decRef(block)   lev_slab_decRefCount(block, 1)

#endif

