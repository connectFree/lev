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

#ifndef _LEV_SLAB_H_
#define _LEV_SLAB_H_

#include <pthread.h>
#include "uv.h"

struct linked_list {
        struct linked_list * next;
        struct linked_list * prev;
};

struct lev_slab_allocator {
        pthread_mutex_t global_lock;
        struct linked_list slabs_list;
        unsigned int object_count;
        unsigned int object_size;
        int canary_check;
};

typedef struct _MemBlock {
    struct lev_slab_allocator *pool;
    int refcount;  /* Number of reference to this block */
    size_t size;   /* Size of the datablock */
    size_t nbytes; /* Number of bytes actually in buffer */
    unsigned char bytes[0];
} MemBlock;

typedef struct _MemSlice {
    MemBlock *mb; /* our MemBlock */
    unsigned char *slice;   /* begining of slice */
    size_t until; /* range of how far we have sliced */
} MemSlice;

void lev_slab_create(struct lev_slab_allocator * allocator,
                 unsigned int objcount,
                 unsigned int objsize,
                 int canary);
void * lev_slab_alloc(struct lev_slab_allocator * allocator);
void lev_slab_free(struct lev_slab_allocator * allocator,
               void * object);

void lev_slab_freePools();
MemBlock *lev_slab_getBlock(size_t size);
int lev_slab_incRef(MemBlock *block);
int lev_slab_decRef(MemBlock *block);



#endif