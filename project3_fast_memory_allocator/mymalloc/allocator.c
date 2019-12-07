/**
 * Copyright (c) 2015 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

/**
 * Implementation of a memory allocator, which can allocate, free, and reallocate
 * memory in the heap.
 *
 * Assumes that no block of memory will ever exceed 2^32 - 1 bytes
 *
 * A "memory block" consists of three sections,
 *  - a header of HEADER_SIZE bytes
 *      currently saves the size of the data section
 *  - a data section of variable size that users can change the value of to save data
 *  - a footer of FOOTER_SIZE bytes
 *      currently saves either
 *        a) the size of the data section if the block is free
 *        b) FOOTER_ALLOC_FLAG if the block is not free
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "./allocator_interface.h"
#include "./memlib.h"

// Don't call libc malloc!
#define malloc(...) (USE_MY_MALLOC)
#define free(...) (USE_MY_FREE)
#define realloc(...) (USE_MY_REALLOC)

// All blocks must have a specified minimum alignment.
// The alignment requirement (from config.h) is >= 8 bytes.
#ifndef ALIGNMENT
#define ALIGNMENT 8
#endif

// Rounds up to the nearest multiple of ALIGNMENT.
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

// The smallest aligned size that will hold a uint32_t value.
static const int SIZE_T_SIZE = (sizeof(uint32_t));
static const int HEADER_SIZE = SIZE_T_SIZE;
static const int FOOTER_SIZE = SIZE_T_SIZE;

// the "size" stored in the footer of the mem block will be 1 if the block is not free
// Note that the size cannot actually ever be 1, so the footer is 1 iff in use
static const size_t FOOTER_ALLOC_FLAG = 1;

// Amount to allocate in my_init to unalign the heap pointer 
// so that the data sections of the mem blocks are aligned
static const int INIT_OFFSET = ALIGN(HEADER_SIZE) - HEADER_SIZE;

/***** FUNCTIONS FOR HEADERS AND FOOTERS *****/
// Convert size from 64 to 32 bit to store in the header/footer
static inline uint32_t mask_size(size_t size) {
  return (uint32_t)size;
}
 
/**
 * Convert size from 32 to 64 bit
 * as the size is stored in 32 bit in the header/footer
 * but actually represents the range of [1, 2^32]
 */
static inline size_t unmask_size(uint32_t size) {
  return (size == 0) ? (1UL << 32) : (size_t)size;
}

/**
 * Get the value in the header of mem block
 * 
 * void* ptr - pointer to the start of the data section of the block of memory
 */
static inline size_t get_header(void* ptr) {
  uint32_t val = *(uint32_t*)((uint8_t*)ptr - HEADER_SIZE);
  return unmask_size(val);
}

/**
 * Set the value in the header of mem block
 * 
 * void* ptr - pointer to the start of the data section of the block of memory
 * size_t value - new value
 */
static inline void set_header(void* ptr, size_t value) {
  *(uint32_t*)((uint8_t*)ptr - HEADER_SIZE) = mask_size(value);
}

/**
 * Get the value in the footer of mem block
 * 
 * void* ptr - pointer to the start of the data section of the block of memory
 * size_t size - size of the data section of the mem block
 */
static inline size_t get_footer(void* ptr, size_t size) {
  uint32_t val = *(uint32_t*)((uint8_t*)ptr + size);
  return unmask_size(val);
}

/**
 * Set the value in the footer of mem block
 * 
 * void* ptr - pointer to the start of the data section of the block of memory
 * size_t size - size of the data section of the mem block
 * size_t value - new value
 */
static inline void set_footer(void* ptr, size_t size, size_t value) {
  *(uint32_t*)((uint8_t*)ptr + size) = mask_size(value);
}

/**
 * Set the value in the footer of mem block
 * 
 * void* ptr - pointer to the start of the data section of the block of memory
 * size_t value - new value
 */
static inline size_t get_last_footer(void* ptr) {
  uint32_t val = *(uint32_t*)((uint8_t*)ptr - HEADER_SIZE - FOOTER_SIZE);
  return unmask_size(val);
}

/** 
 * Check if the given value of the footer indicates that the mem block is free
 */
static inline bool is_free(size_t footer_value) {
  return footer_value != FOOTER_ALLOC_FLAG;
}

/**
 * Check if the mem block is free
 * 
 * void* ptr - pointer to the start of the data section of the block of memory
 * size_t size - size of the data section of the mem block
 */
static inline bool is_free_block(void* ptr, size_t size) {
  return is_free(get_footer(ptr, size));
}
/***** END FUNCTIONS FOR HEADERs AND FOOTERS *****/

/**
 * Computes log_2(x), if x is a power of 2
 * Otherwise, computes log_2(y), where y is the largest power of 2 less than x
 *
 * Equivalently, this function returns the floor of log_2(x)
 */
static inline uint64_t log_2(uint64_t x) {
  return 8 * sizeof(uint64_t) - __builtin_clzl(x) - 1;
}

/**
 * Computes the min power of 2 larger than n
 * Returns n if n is a power of 2
 */
static inline uint32_t next_power_of_two(uint32_t n) {
  return 1 << (log_2(n) + 1);
}

typedef struct FreeList FreeList;

struct FreeList {
  FreeList* prev;
  FreeList* next;
};

// Minimum size of a block
// Must be at least the sum of the sizes of the header, footer, and FreeList
static const size_t MIN_BLOCK_SIZE_ACTUAL = 24;
// Lowest power of 2 that is larger than the minimum block size. 
static const int MIN_BLOCK_SIZE_EXP = 4;
static const uint64_t MIN_BLOCK_SIZE = 1UL << MIN_BLOCK_SIZE_EXP;

// Maximum size of a block (all blocks must have a size < MAX_BLOCK_SIZE)
static const int MAX_BLOCK_SIZE_EXP = 32;
static const uint64_t MAX_BLOCK_SIZE = 1UL << MAX_BLOCK_SIZE_EXP;

// Binned free list uses sizes in the ranges of [2^5, 2^6) to [2^31, 2^32)
// The i-th bin contains the free mem blocks with total sizes in the range [ 2^(i+5), 2^(i+6) )
// with the exception of the first, which is [24, 64)
static const int NUM_FREE_LISTS = MAX_BLOCK_SIZE_EXP - MIN_BLOCK_SIZE_EXP;
FreeList* freelist[NUM_FREE_LISTS];

#ifdef DEBUG
/**
 * Prints out the binned free lists, for debugging purposes
 */
static inline void print_freelists() {
  char* lo = (char*)mem_heap_lo();
  char* hi = (char*)mem_heap_hi() + 1;
  printf("heap_lo: %p, heap_hi: %p\n", lo, hi);

  for (int i = 0; i < NUM_FREE_LISTS; ++i) {
    if (freelist[i] == NULL) continue;

    printf("> Index %d:\n", i);
    for (FreeList* curr = freelist[i]; curr != NULL; curr = curr->next) {
      printf("    Current: %p ", (char*)curr);
      uint32_t size = *(uint32_t*)((uint8_t*)curr - HEADER_SIZE);
      printf("with size %u\n", size);
    }
  }
}
#endif

/**
 * Check the consistency of the free lists
 */
static int my_check_freelists() {
  for (int i = 0; i < NUM_FREE_LISTS; ++i) {
    for (FreeList* curr = freelist[i]; curr != NULL; curr = curr->next) {
      if (curr->next != NULL) {
        if ((uintptr_t)curr->next->prev != (uintptr_t)curr) {
          printf("Index %d - Current node: %lu, Next node: %lu, Next->Prev node: %lu\n", i, (uintptr_t)curr, (uintptr_t)curr->next, (uintptr_t)curr->next->prev);
          printf("Next node's prev pointer doesn't point back to current node\n");
          return -1;
        }
      }
    }
  }
  return 0;
}

/**
 * Get the index of the free list for a memory block with total size block_size
 * 
 * uint32_t block_size - the total size of the mem block
 * 
 * Return the index of the free list for given mem block size
 */
static int get_freelist_index(uint32_t block_size) {
  // if the block size is the max of MAX_BLOCK_SIZE (2^32),
  // then it overflows to 0
  if (block_size == 0) {
    return NUM_FREE_LISTS - 1;
  }
  if (block_size <= MIN_BLOCK_SIZE) {
    if (block_size==0){
      return NUM_FREE_LISTS - 1;
    }
    return 0;
  }
  // adjusts the block_size to the largest power of 2 <= block_size
  // This is the power of two that starts the range for the bin
  // that the block_size belongs to
  uint32_t largest_power_of_two = next_power_of_two(block_size + 1) / 2;
  return log_2(largest_power_of_two / MIN_BLOCK_SIZE);
}

/**
 * Add the free memory block into the specified free list
 * 
 * void* ptr - pointer to the start of the data section of the block of memory
 * int index - index of the free list to add the mem block to
 */
static inline void add_to_free_list(void* ptr, int index) {
  FreeList* new_start = (FreeList*) ptr;

  // If the specified free list is not empty,
  // update the prev field of the current start of the free list
  if (freelist[index] != NULL) {
    freelist[index]->prev = new_start;
  }

  new_start->prev = NULL;
  new_start->next = freelist[index];
  freelist[index] = new_start;
  assert(freelist[index]->next == NULL || freelist[index]->next->prev == freelist[index]);
}

/**
 * Given a memory block that was just freed, split the memory block into sizes
 *   stored in the free lists, marking each as free in the footer in the process
 * 
 * Precondition:
 *  - the total size of the given memory block should be a multiple of MIN_BLOCK_SIZE
 *  - the header of the given memory block (which is before the ptr) is set correctly
 * 
 * void* ptr - pointer to the start of the data section of the block of memory
 */
static inline void add_free_list(void* ptr) {
  uint32_t size = get_header(ptr);
  uint32_t block_size = size + HEADER_SIZE + FOOTER_SIZE;

  // Some checks regarding size of the memory block
  assert(block_size >= MIN_BLOCK_SIZE);

  // Update the footer of the new block to indicate that is is free
  set_footer(ptr, size, size);
  // Add the new block to the corresponding free list
  int freelist_index = get_freelist_index(block_size);
  assert(freelist_index >= 0 && freelist_index < NUM_FREE_LISTS);

  #ifdef DEBUG
  if (my_check_freelists() == -1) {
    printf("before add_to messed up\n");
  }
  #endif

  add_to_free_list(ptr, freelist_index);

  #ifdef DEBUG
  if (my_check_freelists() == -1) {
    printf("add messed up\n");
  }
  #endif
}

/**
 * Remove the FreeList node at the given pointer from its free list
 * 
 * void* ptr - pointer to the start of the data section of the block of memory
 *   which is also points to the FreeList node for the mem block
 */
static inline void remove_free_list(void* ptr) {
  // Check that the mem block is free
  assert(is_free_block(ptr, get_header(ptr)));

  FreeList* node = (FreeList*)ptr;

  // If not the end of the free list, update the prev field of the next node in the free list
  if (node->next != NULL) {
    assert(node == node->next->prev);
    node->next->prev = node->prev;
  }

  // Update the "node" before the given, whether it's the freelist pointer or a previous node
  if (node->prev == NULL) {
    uint32_t block_size = HEADER_SIZE + get_header((void*)node) + FOOTER_SIZE;
    int index = get_freelist_index(block_size);
    freelist[index] = node->next;
  } else {
    node->prev->next = node->next;
  }

  #ifdef DEBUG
  if (my_check_freelists() == -1) {
    printf("remove messed up\n");
  }
  #endif
}

// check - This checks our invariant that the uint32_t header before every
// block points to either the beginning of the next block, or the end of the
// heap.
int my_check() {
  char* p;
  char* lo = (char*)mem_heap_lo();
  char* hi = (char*)mem_heap_hi() + 1;
  size_t size = 0;

  p = lo;
  p = p + INIT_OFFSET;
  while (lo <= p && p < hi) {
    size = unmask_size(*(uint32_t*)p) + HEADER_SIZE + FOOTER_SIZE;
    p += size;
    // printf("size: %lu, p: %p\n", size, p);
  }

  if (p != hi) {
    printf("Bad headers did not end at heap_hi!\n");
    printf("heap_lo: %p, heap_hi: %p, size: %zu, p: %p\n", lo, hi, size, p);
    return -1;
  }

  if (my_check_freelists() == -1) {
    return -1;
  }

  return 0;
}

// init - Initialize the malloc package.  Called once before any other
// calls are made.  Since this is a very simple implementation, we just
// return success.
int my_init() {
  assert(MIN_BLOCK_SIZE_ACTUAL >= HEADER_SIZE + sizeof(FreeList) + FOOTER_SIZE);

  for (int i = 0; i < NUM_FREE_LISTS; i++) {
    freelist[i] = NULL;
  }
  // unalign the heap pointer so that the data sections of the mem blocks are aligned
  mem_sbrk(INIT_OFFSET);
  return 0;
}

/**
 * Given the (free) memory block (represented by the given pointer to the data section),
 * return a pointer to a memory block of at least a total size of block_size,
 * and add the extra memory as a new block back to the binned free list
 */
void* split_block(void* ptr, size_t block_size) {
  // if the block from the free list is larger than necessary,
  // add back the rest as a new mem block
  size_t full_size = get_header(ptr);
  size_t full_block_size = full_size + HEADER_SIZE + FOOTER_SIZE;

  assert(full_block_size - block_size >= 0);

  // size of the returned pointer
  size_t returned_size = full_size;
  
  // Split off excess memory into a new block, if there's enough
  if (full_block_size - block_size >= MIN_BLOCK_SIZE_ACTUAL) {
    void* extra_mem = (void*)((uint8_t*)ptr + block_size);
    // update the size in the header
    // size of extra mem block is full_size - size of new block data section - HEADER - FOOTER
    // which is full_size - (block_size - HEADER - FOOTER) - HEADER - FOOTER
    set_header(extra_mem, full_size - block_size);
    // footer is updated in add_free_list
    add_free_list(extra_mem);

    // reduce size of the returned block from split
    returned_size = block_size - HEADER_SIZE - FOOTER_SIZE;
  }

  // update header size
  set_header(ptr, returned_size);
  // mark in the footer that the block is in use
  set_footer(ptr, returned_size, FOOTER_ALLOC_FLAG);

  #ifdef DEBUG
  if (my_check() == -1) {
    printf("Inconsistent after splitting block\n");
  }
  #endif

  return ptr;
}

// return pointer to free block taken from a free list if possible, otherwise return NULL
void* freelist_malloc(size_t block_size) {
  assert(block_size > 0);
  size_t freelist_size = MIN_BLOCK_SIZE_ACTUAL;
  int i = 0;
  while (i < NUM_FREE_LISTS) {
    if (block_size <= freelist_size) {
      for (int j = i; j < NUM_FREE_LISTS; j++) {
        if (freelist[j] == NULL) continue;

        FreeList* p = freelist[j];
        remove_free_list(p);
        return split_block(p, block_size);
      }
      break;
    }
    i++;
    freelist_size = (i==1) ? MIN_BLOCK_SIZE : freelist_size;
    freelist_size *= 2;
  }

  // If we reached this point, then that means the binned free lists did not contain a mem block
  // with enough space for the requested block size, and we return NULL
  return NULL;
}

//  malloc - Allocate a block by incrementing the brk pointer.
//  Always allocate a block whose size (in bytes) is a multiple of the alignment.
void* my_malloc(size_t size) {
  // We allocate a little bit of extra memory so that we can store the
  // size of the block we've allocated.  Take a look at realloc to see
  // one example of a place where this can come in handy.
  size_t aligned_size = ALIGN(size + HEADER_SIZE + FOOTER_SIZE);
  
  // Round up to nearest multiple of 32
  size_t block_size = (aligned_size < MIN_BLOCK_SIZE_ACTUAL) ? MIN_BLOCK_SIZE_ACTUAL : aligned_size;

  #ifdef DEBUG
  size_t proposed_size = (aligned_size < MIN_BLOCK_SIZE_ACTUAL) ? MIN_BLOCK_SIZE_ACTUAL : aligned_size;
  if (proposed_size > MAX_BLOCK_SIZE) {
    // size is too big for our implementation
    printf("Block size %lu for requested size %zu is too large for our implementation\n", proposed_size, size);
    return NULL;
  }
  #endif

  void* p = freelist_malloc(block_size);
  if (p != NULL) {
    // found a block of memory in the free list to use
    return p;
  }

  // if the last mem block on the heap is free, expand its size and use it
  // instead of making an entirely new mem block
  uint32_t last_block_footer = *(uint32_t*)(mem_heap_hi() + 1 - FOOTER_SIZE);
  size_t last_block_size = unmask_size(last_block_footer);
  if (last_block_size != FOOTER_ALLOC_FLAG && block_size > last_block_size){
    size_t data_size = block_size - HEADER_SIZE - FOOTER_SIZE;
    size_t extra_mem_needed = data_size - last_block_size; 

    p = mem_heap_hi() + 1 - FOOTER_SIZE - last_block_size;
    remove_free_list(p);
    if (extra_mem_needed > 0){
      mem_sbrk(extra_mem_needed);
    }
    set_header(p, data_size);
    set_footer(p, data_size, FOOTER_ALLOC_FLAG);
    return p;
  }

  // Otherwise, we need more memory
  // Expand the heap by the given number of bytes and returns a pointer to
  // the newly-allocated area.  This is a slow call, so you will want to
  // make sure you don't wind up calling it on every malloc.
  p = mem_sbrk(block_size);

  if (p == (void*)-1) {
    // Whoops, an error of some sort occurred.  We return NULL to let
    // the client code know that we weren't able to allocate memory.
    return NULL;
  } else {
    // We store the size of the data area of the block we've allocated in the header
    // which is the first HEADER_SIZE bytes.
    *(uint32_t*)p = block_size - HEADER_SIZE - FOOTER_SIZE;
    // Mark that the block is in use in the footer, which is the last FOOTER_SIZE bytes
    *(uint32_t*)((uint8_t*)p + block_size - FOOTER_SIZE) = FOOTER_ALLOC_FLAG;

    // Then, we return a pointer to the rest of the block of memory,
    // which is at least size bytes long.  We have to cast to uint8_t
    // before we try any pointer arithmetic because voids have no size
    // and so the compiler doesn't know how far to move the pointer.
    // Since a uint8_t is always one byte, adding SIZE_T_SIZE after
    // casting advances the pointer by SIZE_T_SIZE bytes.
    return (void*)((char*)p + HEADER_SIZE);
  }
}

/**
 * Coalesce the block of memory that the given pointer corresponds to
 *   with adjacent free memory blocks
 * 
 * void* ptr - pointer to the start of the data section of the block of memory
 * size_t block_size - size (in bytes) of the given memory block
 * 
 * Returns the pointer to the start of the data section of the coalesced memory block
 *   Note that this function does not update the footer value - this is done by add_free_list()
 */
void* coalesce(void* ptr, size_t block_size) {
  // Flag for if coalescing happened in last loop
  // need to check adjacent mem blocks for possible coalescing
  bool done = false;

  while (!done) {
    done = true;

    // Try mem block in front
    if ((uintptr_t)mem_heap_lo() <= (uintptr_t)((uint8_t*)ptr - HEADER_SIZE - MIN_BLOCK_SIZE_ACTUAL)) {
      // prev_data_size = FOOTER_ALLOC_FLAG if in use;
      //                  size of data section of the free block in front o.w.
      size_t prev_data_size = get_last_footer(ptr);
      if (is_free(prev_data_size)) {  // check if previous block is free
        done = false;

        // remove prev block from free list
        void* prev_ptr = (void*) ((uint8_t*)ptr - HEADER_SIZE - FOOTER_SIZE - prev_data_size);
        remove_free_list(prev_ptr);

        // set ptr to the start of data section of the new coalesced block
        ptr = prev_ptr;

        // Update the header and footer of the new coalesced block
        //   with the size of the coalesced's data section
        // The size of the coalesced block's data section is
        //   prev_data_size + FOOTER_SIZE + HEADER_SIZE + curr_data_size
        //     = prev_data_size + block_size
        size_t new_size = prev_data_size + block_size;
        set_header(ptr, new_size);
        set_footer(ptr, new_size, new_size);

        // update (other) loop params
        block_size = HEADER_SIZE + new_size + FOOTER_SIZE;
      }
    }

    #ifdef DEBUG
    if (my_check() == -1) {
      printf("Inconsistent after coalesce with block in front\n");
    }
    #endif

    // Try mem block behind
    if ((uintptr_t)mem_heap_hi() >= (uintptr_t)((uint8_t*)ptr - HEADER_SIZE + block_size + MIN_BLOCK_SIZE_ACTUAL)) {
      void* next_ptr = (void*) ((uint8_t*)ptr + block_size);
      size_t next_data_size = get_header(next_ptr);
      if (is_free_block(next_ptr, next_data_size)) {  // check if next block is free
        done = false;

        // Remove next block from free list
        remove_free_list(next_ptr);

        // Set ptr to the start of data section of the new coalesced block (no change)

        // Update the header and footer of the new coalesced block
        //   with the size of the coalesced's data section
        // The size of the coalesced block's data section is
        //   curr_data_size + FOOTER_SIZE + HEADER_SIZE + next_data_size
        //     = block_size + next_data_size
        size_t new_size = block_size + next_data_size;
        set_header(ptr, new_size);
        set_footer(ptr, new_size, new_size);

        // update (other) loop params
        block_size = HEADER_SIZE + new_size + FOOTER_SIZE;
      }
    }

    #ifdef DEBUG
    if (my_check() == -1) {
      printf("Inconsistent after coalesce with block behind\n");
    }
    #endif
  }

  return ptr;
}

/**
 * Free the block of memory that the given pointer corresponds to
 *   adding it to the corresponding free list(s).
 * 
 * Coalesces the pointer with adjacent free blocks, then 
 *   breaking it into sizes in the free lists when adding
 * 
 * void* ptr - pointer to the start of the data section of the block of memory
 */
void my_free(void* ptr) {
  // block_size = (size of the data area of the mem block) + size of header + size of footer
  size_t block_size = get_header(ptr) + HEADER_SIZE + FOOTER_SIZE;
  // returned pointer points to start of data area of the mem block
  ptr = coalesce(ptr, block_size);
  add_free_list(ptr);
}

// realloc - Implemented simply in terms of malloc and free
void* my_realloc(void* ptr, size_t size) {
  void* newptr;
  size = ALIGN(size);
  // Get the size of the old block of memory (data part).
  // We stashed this in the HEADER_SIZE bytes directly before the
  // address we returned.  Now we can back up by that many bytes and read
  // the size.
  size_t copy_size = get_header(ptr);

  // If new size is less than current, we can just return the current pointer
  if (size < copy_size) {
    // when we split off blocks using split_block(), we find that the
    // code performs worse on c10, which looks at this exact case
    return ptr;
  }

  // If the block is at the end of the heap, just expand its size
  void* block_end = (char*) ptr + copy_size + FOOTER_SIZE;
  if (block_end - 1 == my_heap_hi()){
    size_t new_mem_needed = ALIGN(size - copy_size);
    mem_sbrk(new_mem_needed);
    // update header with new size
    set_header(ptr, copy_size + new_mem_needed);
    set_footer(ptr, copy_size + new_mem_needed, FOOTER_ALLOC_FLAG);
    return ptr;  
  }

  // Allocate a new chunk of memory, and fail if that allocation fails.
  newptr = my_malloc(size);
  if (NULL == newptr) {
    return NULL;
  }

  // If the new block is smaller than the old one, we have to stop copying
  // early so that we don't write off the end of the new block of memory.
  if (size < copy_size) {
    copy_size = size;
  }

  // This is a standard library call that performs a simple memory copy.
  memcpy(newptr, ptr, copy_size);

  // Release the old block.
  my_free(ptr);

  // Return a pointer to the new block.
  return newptr;
}
