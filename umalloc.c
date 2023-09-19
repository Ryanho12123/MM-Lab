#include "umalloc.h"
#include "csbrk.h"
#include <stdio.h>
#include <assert.h>
#include "ansicolors.h"

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Ryan Ho rdh2837" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;

/*
 * is_allocated - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc |= 0x1;
}


/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc &= ~0x1;
}

/*
 * get_size - gets the size of the block.
 */
size_t get_size(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & ~(ALIGNMENT-1);
}

/*
 * get_next - gets the next block.
 */
memory_block_t *get_next(memory_block_t *block) {
    assert(block != NULL);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields, along with NUlling out the next 
 * field.
 */
void put_block(memory_block_t *block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;
    block->next = NULL;
}

/*
 * get_payload - gets the payload of the block.
 */
void *get_payload(memory_block_t *block) {
    assert(block != NULL);
    return (void*)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t *get_block(void *payload) {
    assert(payload != NULL);
    return ((memory_block_t *)payload) - 1;
}

/*
 * The following are helper functions that can be implemented to assist in your
 * design, but they are not required. 
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) {
    //* STUDENT TODO
    memory_block_t *current_block = free_head;
    while (current_block != NULL) {
        // Check if the current block is large enough
        if (get_size(current_block) >= size) {
            return current_block; // Found a suitable block
        }
        current_block = get_next(current_block); // Move to the next block in the free list
    }
    
    // No suitable block found
    return NULL;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    //* STUDENT TODO
    size_t total_size = size + sizeof(memory_block_t);
    
    // Request additional memory from the operating system
    void *new_heap = csbrk(total_size);
    
    // Check if the csbrk call failed (returned NULL)
    if (new_heap == NULL) {
        return NULL; // Failed to extend the heap
    }
    
    // Create a new memory block structure for the extended heap
    memory_block_t *new_block = (memory_block_t *)new_heap;
    
    // Mark the new block as free and set its size
    put_block(new_block, total_size, 0);
    
    // Add the new block to the free list
    new_block->next = free_head;
    free_head = new_block;
    
    return new_block; // Return the extended block
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    //* STUDENT TODO
    // assert(block != NULL);

    // size_t block_size = get_size(block);

    // // Check if splitting is possible
    // if (block_size < size + sizeof(memory_block_t)) {
    //     return NULL;  // Cannot split, not enough space for a new block.
    // }

    // // Calculate the size of the new free block.
    // size_t new_block_size = block_size - size;

    // // Ensure the new free block size is aligned.
    // new_block_size = ALIGN(new_block_size);

    // // Ensure the new free block size is at least as large as the minimum block size.
    // if (new_block_size < sizeof(memory_block_t)) {
    //     new_block_size = sizeof(memory_block_t);
    // }

    // // Calculate the size of the allocated block.
    // size_t allocated_size = block_size - new_block_size;

    // // Create a new free block from the remaining space.
    // memory_block_t *new_block = (memory_block_t *)((char *)block + allocated_size);
    // put_block(new_block, new_block_size, false);  // Mark it as free.

    // // Update the size of the original block.
    // put_block(block, allocated_size, true);  // Mark it as allocated.

    // // Update the linked list pointers.
    // new_block->next = block->next;
    // block->next = new_block;

    // return new_block;  // Return the new free block.
     assert(block != NULL);
    size_t block_size = get_size(block);
    
    // Calculate the remaining free block size.
    size_t remaining_size = block_size - size - sizeof(memory_block_t);

    // Check if splitting is possible.
    if (remaining_size < sizeof(memory_block_t)) {
        return NULL;  // Not enough space for a new free block.
    }

    // Create the allocated block from the beginning of the original block.
    put_block(block, size, true);

    // Create a new free block from the remaining space.
    memory_block_t *new_free_block = (memory_block_t *)((char *)block + size + sizeof(memory_block_t));
    put_block(new_free_block, remaining_size, false);
    
    // Update the linked list pointers.
    new_free_block->next = block->next;
    block->next = new_free_block;

    return block;  // Return the allocated block.

}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t *coalesce(memory_block_t *block) {
    //* STUDENT TODO
    assert(block != NULL);

    memory_block_t *next_block = (memory_block_t *)((char *)block + get_size(block));

    if (!is_allocated(next_block)) {
        // Next block is free, coalesce with it.
        block->next = next_block->next;
        block->block_size_alloc += next_block->block_size_alloc;
    }

    // TODO: Implement coalescing with the previous block if applicable.

    return block;
}



/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    //* STUDENT TODO
    size_t initial_size = 8192;

    void *initial_heap = csbrk(initial_size);
    if (initial_heap == NULL) {
        return -1; 
    }

    memory_block_t *initial_block = (memory_block_t *)initial_heap;
    put_block(initial_block, initial_size, 0); 

    free_head = initial_block;

    return 0; 
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    //* STUDENT TODO
    // Check for invalid size
    if (size == 0) {
        return NULL;
    }

    // Find a suitable free block
    memory_block_t *block = find(size);

    if (block == NULL) {
        // No suitable free block found, extend the heap
        block = extend(size);
        if (block == NULL) {
            return NULL; // Heap extension failed
        }
    }

    // Allocate the block
    allocate(block);

    // Return a pointer to the allocated memory
    return get_payload(block);
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    //* STUDENT TODO
    // Check for NULL pointer
    if (ptr == NULL) {
        return;
    }

    // Get the corresponding memory block
    memory_block_t *block = get_block(ptr);

    // Mark the block as unallocated
    deallocate(block);

    // Optionally, coalesce the block
    coalesce(block);
}
