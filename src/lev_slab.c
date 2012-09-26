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

#include <stdlib.h>
#include <stdio.h>
#include "lev_slab.h"

#ifndef MIN
  #define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

const static lev_slab_allocator_t mem_1k;
const static lev_slab_allocator_t mem_8k;
const static lev_slab_allocator_t mem_16k;
const static lev_slab_allocator_t mem_64k;
const static lev_slab_allocator_t mem_1024k;

#define lev_slab_offsetof(type, member) ((unsigned long)&((type *)0)->member)

static void corrupted_canary(const void * object) __attribute__((noreturn));
static void corrupted_canary(const void * object) {
  printf("memory overrun on object %p\n", object);
  abort();
}


static void _lev_slab_fill(lev_slab_allocator_t *allocator, size_t blocksize, int min_number) {
  allocator->pool_count = 0;
  allocator->pool_min = MIN(min_number, SLAB_MAXFREELIST);
  MemBlock *mb;  
  while (allocator->pool_count < SLAB_MAXFREELIST) {
    /* 
      while() uses SLAB_MAXFREELIST instead of allocator->pool_min
      to suppress bogus warning: "array subscript is above array bounds"
    */
    if (allocator->pool_count == allocator->pool_min) {
      break;
    }
    mb = (MemBlock *)malloc(sizeof(MemBlock) + blocksize);
    allocator->pool[allocator->pool_count++] = mb;
  }
  
}

void lev_slab_fill() {
  _lev_slab_fill((lev_slab_allocator_t*)&mem_1k, 1024, 1024);
  _lev_slab_fill((lev_slab_allocator_t*)&mem_8k, 8*1024, 512);
  _lev_slab_fill((lev_slab_allocator_t*)&mem_16k, 16*1024, 8);
  _lev_slab_fill((lev_slab_allocator_t*)&mem_64k, 64*1024, 8);
  _lev_slab_fill((lev_slab_allocator_t*)&mem_1024k, 1024*1024, 0);
}

MemBlock *lev_slab_getBlock(size_t size) {
  lev_slab_allocator_t* allocator;
  MemBlock *block;
  int blocksize;

  if (size <= 1024) {
    allocator = (lev_slab_allocator_t*)&mem_1k;
    blocksize = 1024;
  } else if (size <= 8*1024) {
    allocator = (lev_slab_allocator_t*)&mem_8k;
    blocksize = 8*1024;
  } else if (size <= 16*1024) {
    allocator = (lev_slab_allocator_t*)&mem_16k;
    blocksize = 16*1024;
  } else if (size <= 64*1024) {
    allocator = (lev_slab_allocator_t*)&mem_64k;
    blocksize = 64*1024;
  } else if (size <= 1024*1024) {
    allocator = (lev_slab_allocator_t*)&mem_1024k;
    blocksize = 1024*1024;
  } else {
    return NULL;
  }

  if (allocator->pool_count) {
    block = allocator->pool[--allocator->pool_count];
    /*printf("BLOCK TAKEN FROM POOL %d\n", blocksize);*/
  } else {
    /*printf("BLOCK TAKEN FROM MALLOC %d\n", blocksize);*/
    block = (MemBlock *)malloc(sizeof(MemBlock) + blocksize);
  }

  /*printf("[%p] lev_slab_getBlock(size=%d)\n", block, blocksize);*/
  block->allocator = allocator;
  block->refcount = 0;
  block->size = blocksize;
  block->nbytes = 0;
  return block;
}

int lev_slab_incRef(MemBlock *block) {
  block->refcount++;
  /*printf("[%p] lev_slab_incRef(r=%d, p=%p(%lu))\n", block, block->refcount, block->allocator, block->size);*/
  return block->refcount;
}

int lev_slab_decRefCount(MemBlock *block, int count) {
  if (!block) return 0;

  block->refcount--;

  /*printf("[%p] lev_slab_decRef(r=%d, p=%p(%lu))\n", block, block->refcount, block->allocator, block->size);*/
  
  if (block->refcount == 0) {/* return block to pool */
    if (block->allocator->pool_count < block->allocator->pool_min) {
      block->allocator->pool[block->allocator->pool_count++] = block;
      /*printf("BLOCK PUT ON SHELF %lu -> %d\n", block->size, block->allocator->pool_count); */
    } else {
      /*printf("FREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREE\n");*/
      free(block); 
    }
    return 0;
  }
  return block->refcount;
}
