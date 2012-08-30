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

#include <stdlib.h>
#include <stdio.h>
#include "lev_slab.h"

struct object_slab {
        struct linked_list slabs_list;
        void * ready_list;
        unsigned int used_objects;
        unsigned long object_memory[];
};

const static struct lev_slab_allocator* mem_1k = NULL;
const static struct lev_slab_allocator* mem_8k = NULL;
const static struct lev_slab_allocator* mem_16k = NULL;
const static struct lev_slab_allocator* mem_64k = NULL;
const static struct lev_slab_allocator* mem_1024k = NULL;

#define lev_slab_offsetof(type, member) ((unsigned long)&((type *)0)->member)

static void corrupted_canary(const void * object) __attribute__((noreturn));
static void corrupted_canary(const void * object) {
  printf("memory overrun on object %p\n", object);
  abort();
}

void lev_slab_create(struct lev_slab_allocator * allocator,
                 unsigned int objcount,
                 unsigned int objsize,
                 int canary)
{
  pthread_mutex_init(&allocator->global_lock, NULL);
  allocator->slabs_list.next = &allocator->slabs_list;
  allocator->slabs_list.prev = &allocator->slabs_list;

  objsize +=   sizeof(void *) - 1;
  objsize &= ~(sizeof(void *) - 1);
  canary = canary != 0;

  allocator->object_count = objcount;
  allocator->object_size  = objsize;
  allocator->canary_check = canary;
}

void * lev_slab_alloc(struct lev_slab_allocator * allocator) {
  struct linked_list * list;
  struct object_slab * slab;
  void * before;
  void * object;

  pthread_mutex_lock(&allocator->global_lock);

  list = allocator->slabs_list.next;
  if (list == &allocator->slabs_list) {
    unsigned int i;

    slab = malloc(
             (
               (sizeof(void *) << allocator->canary_check)
               + allocator->object_size
             )
             * allocator->object_count
             + sizeof(struct object_slab)
           );

    if (slab == NULL) {
      object = NULL;
      goto unlock_finish;
    }

    allocator->slabs_list.next = &slab->slabs_list;
    allocator->slabs_list.prev = &slab->slabs_list;
    slab->slabs_list.next = &allocator->slabs_list;
    slab->slabs_list.prev = &allocator->slabs_list;

    before = &slab->ready_list;
    object = slab->object_memory;
    for (i = 0; i < allocator->object_count; i++) {
      *((void **)before) = object;
      before = object;
      object += allocator->object_size;
      if (allocator->canary_check != 0) {
        *((void **)object) = allocator;
        object += sizeof(void *);
      }
      *((void **)object) = slab;
      object += sizeof(void *);
    }
    *((void **)before) = NULL;

    slab->used_objects = 0;
  } else {
    slab = (void *)list - lev_slab_offsetof(struct object_slab, slabs_list);
  }

  slab->used_objects++;
  object = slab->ready_list;
  slab->ready_list = *((void **)object);

  if (slab->ready_list == NULL) {
    slab->slabs_list.next->prev = slab->slabs_list.prev;
    slab->slabs_list.prev->next = slab->slabs_list.next;
  }

unlock_finish:
  pthread_mutex_unlock(&allocator->global_lock);

  return object;
} /* X:E lev_slab_alloc */

void lev_slab_free(struct lev_slab_allocator * allocator, void * object)
{
  void * lookup;
  struct object_slab * slab;
  struct object_slab * comp;
  struct linked_list * next;
  struct linked_list * prev;

  lookup = object;
  lookup += allocator->object_size;
  if (allocator->canary_check != 0) {
    if (*((void **)lookup) != allocator) {
      corrupted_canary(object);
    }
    lookup += sizeof(void *);
  }
  slab = *((void **)lookup);

  pthread_mutex_lock(&allocator->global_lock);

  slab->used_objects--;
  if (slab->ready_list != NULL) {
    next = slab->slabs_list.next;
    prev = slab->slabs_list.prev;
    if (next == &allocator->slabs_list) {
      goto insert_obj;
    }
    next->prev = prev;
    prev->next = next;
  } else {
    prev = &allocator->slabs_list;
    next = prev->next;
    goto insert_slab;
  }

  for (;;) {
    comp = NULL;
    if (next == &allocator->slabs_list) {
      break;
    }
    comp = (void *)next - lev_slab_offsetof(struct object_slab, slabs_list);
    if (slab->used_objects >= comp->used_objects) {
      break;
    }
    prev = next;
    next = next->next;
  }

  if (comp != NULL &&
      comp->used_objects == 0 &&
      slab->used_objects == 0
     ) {
    goto unlock_finish;
  }
    

insert_slab:
  next->prev = &slab->slabs_list;
  prev->next = &slab->slabs_list;
  slab->slabs_list.next = next;
  slab->slabs_list.prev = prev;

insert_obj:
  *((void **)object) = slab->ready_list;
  slab->ready_list = object;
  slab = NULL;

unlock_finish:
  pthread_mutex_unlock(&allocator->global_lock);

  if (slab != NULL) {
    free(slab);
  }
} /* X:E lev_slab_free */

MemBlock *lev_slab_getBlock(size_t size) {
  struct lev_slab_allocator* pool;
  int blocksize;

  if (size <= 1024) {
    pool = (struct lev_slab_allocator*)mem_1k;
    blocksize = 1024;
  } else if (size <= 8*1024) {
    pool = (struct lev_slab_allocator*)mem_8k;
    blocksize = 8*1024;
  } else if (size <= 16*1024) {
    pool = (struct lev_slab_allocator*)mem_16k;
    blocksize = 16*1024;
  } else if (size <= 64*1024) {
    pool = (struct lev_slab_allocator*)mem_64k;
    blocksize = 64*1024;
  } else if (size <= 1024*1024) {
    pool = (struct lev_slab_allocator*)mem_1024k;
    blocksize = 1024*1024;
  } else {
    return NULL;
  }

  if (!pool) { /* init our pool */
    pool = malloc(sizeof(struct lev_slab_allocator));
    lev_slab_create(
       pool
      ,1
      ,sizeof(MemBlock) + blocksize
      ,1
      );
  }

  MemBlock *block = lev_slab_alloc(pool);
  printf("[%p] lev_slab_getBlock(p=%p)\n", block, pool);
  block->pool = pool;
  block->refcount = 0;
  block->size = blocksize;
  block->nbytes = 0;
  return block;
}

int lev_slab_incRef(MemBlock *block) {
  block->refcount++;
  return block->refcount;
}

int lev_slab_decRef(MemBlock *block) {
  if (!block) return 0;

  block->refcount--;

  printf("[%p] lev_slab_decRef(r=%d, p=%p)\n", block, block->refcount, block->pool);
  
  if (block->refcount == 0) {/* return block to pool */
    lev_slab_free(block->pool, block);
  }

  return block->refcount;
}
