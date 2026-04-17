#ifndef __KERNEL_SLAB_ALLOCATOR_H
#define __KERNEL_SLAB_ALLOCATOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SLAB_MAGIC 0xCAFEBABEDEADBABE
#define OBJECT_POISON 0xDEADDEADDEADDEAD

// We use this to make a linked list but it is not metadata
// once we allocate the object the data will just run over the pointer
// for example: 64 byte block, 8 bytes for pointer but once we allocate its
// those initial 8 bytes at the start will just be ran over with user data
struct free_object {
    uint64_t magic;
    struct free_object *next;
};

// 1 slab = 2 pages and slab contains smaller objects within
// slab header is placed at the beginning of each page
struct slab {
    uint64_t magic;
    struct slab_cache *cache;      // points back to slab cache
    uint64_t phys_addr;            
    size_t free_count;             
    struct free_object *free_list; 
    struct slab *next;             
};

// Essentially a slab manager for different sized slabs 
struct slab_cache {
    size_t object_size;            // Size of objects in this cache
    size_t objects_per_slab;       // Number of objects per slab
    size_t slab_size;              // Size of each slab (usually PAGE_FRAME_SIZE)
                                   // but we use 2*PAGE_FRAME_SIZE
    
    // Slab lists
    struct slab *full_slabs;       // Slabs with no free objects
    struct slab *partial_slabs;    // Slabs with some free objects
    struct slab *empty_slabs;      // Slabs with all objects free
    
    // Statistics
    size_t total_objects;          // Total objects across all slabs
    size_t allocated_objects;      // Currently allocated objects
    size_t total_slabs;            // Total number of slabs
};



// We create caches for all slab sizes 
void slab_allocator_init(void);
void slab_cache_init(struct slab_cache *cache, size_t object_size);

size_t calculate_objects_per_slab(size_t object_size);
struct slab *slab_create(struct slab_cache *cache);

void *slab_alloc(struct slab_cache *cache);
void slab_free(struct slab *slab, void *ptr);
void *slab_alloc_size(size_t size);
bool is_slab_address(void *ptr);

struct slab *slab_find_containing(void *ptr);
void slab_remove_from_list(struct slab *target_slab, struct slab_cache *cache);

// Remove the slab and return the page to buddy allocator
void slab_destroy(struct slab *slab);

void slab_cache_shrink(struct slab_cache *cache);

void slab_print_cache_stats(struct slab_cache *cache);
void slab_print_all_stats(void);

#endif
