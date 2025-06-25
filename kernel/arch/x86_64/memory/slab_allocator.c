#include <kernel/slab_allocator.h>
#include <kernel/buddy_allocator.h>

static size_t slab_sizes[] = {
    16, 32, 64, 128, 256, 512, 1024, 2048  
};

#define NUM_SLAB_SIZES (sizeof(slab_sizes) / sizeof(slab_sizes[0]))
static struct slab_cache slab_caches[NUM_SLAB_SIZES];

void slab_allocator_init(void){
    kprintf("Initializing slab allocator...\n");
    
    for(size_t i = 0; i < NUM_SLAB_SIZES; i++){ 
         slab_cache_init(&slab_caches[i], slab_sizes[i]);
         kprintf("Slab cache initialized for size %lu bytes\n", slab_sizes[i]);
    }

    KSUCCESS("Slab allocator initialized successfully\n");
}

void slab_cache_init(struct slab_cache *cache, size_t object_size){
    cache->object_size = object_size;
    cache->objects_per_slab = calculate_objects_per_slab(object_size);
    cache->slab_size = 2 * PAGE_FRAME_SIZE; 
    
    // Initialize lists
    cache->full_slabs = NULL;
    cache->partial_slabs = NULL;
    cache->empty_slabs = NULL;
    
    cache->total_objects = 0;
    cache->allocated_objects = 0;
    cache->total_slabs = 0;
    
    kprintf("Cache initialized: object_size=%lu, objects_per_slab=%lu\n", 
          object_size, cache->objects_per_slab);

}

size_t calculate_objects_per_slab(size_t object_size){
    // Slab allocator takes one page from buddy allocator and divides it up 
    // but we need to store the slab header at the start of the slab for metadata 
    // hence why we calculate total object count like this
    size_t usable_space = (2 * PAGE_FRAME_SIZE) - sizeof(struct slab);
    return usable_space / object_size;
}

struct slab *slab_create(struct slab_cache *cache){
    uint64_t phys_addr = buddy_alloc_pages(1);
    if(phys_addr == 0){
        KERROR("Failed to allocate page for a new slab\n");
        return NULL;
    }

    void *virt_addr = phys_to_virt(phys_addr);
    struct slab *slab = (struct slab *)virt_addr;

    slab->cache = cache;
    slab->phys_addr = phys_addr;
    slab->free_count = cache->objects_per_slab;
    slab->next = NULL;
    slab->magic = SLAB_MAGIC;
    slab->free_list = NULL;

    // Must cast to char * since C doesn't allow void pointer arithemtic
    char *objects_start = (char *)virt_addr + sizeof(struct slab);

    for(size_t i = 0; i < cache->objects_per_slab; i++){
        // Buckle the fuck up:
        // Our free_object can be used for an arbitrary slab object size and we
        // need to calculate with that in mind, so we take the start of our objects 
        // and then to that we add the ith object times the size of our objects which 
        // we get from the cache, that's how we get the ith object
        struct free_object *obj = (struct free_object *)(objects_start + i * cache->object_size);
        // Good ol' link at the head
        obj->next = slab->free_list;
        slab->free_list = obj;
    }

    // Another good ol' link at the head
    slab->next = cache->empty_slabs;
    cache->empty_slabs = slab;

    // Nerd statistics 
    cache->total_slabs++;
    cache->total_objects += cache->objects_per_slab;

    kprintf("Created new slab for cache (object_size=%lu)\n", cache->object_size);
    return slab;
}

void *slab_alloc(struct slab_cache *cache){
    struct slab *slab = NULL;
     
    if(cache->partial_slabs)
        slab = cache->partial_slabs;
    else if(cache->empty_slabs)
        slab = cache->empty_slabs;
    else {
        slab = slab_create(cache);
        if(!slab)
            return NULL;
    }

    if(slab->free_list == NULL){
        KERROR("Slab was in free list but the free list is NULL\n");
        return NULL;
    }

    // Good ol' unlink the head 
    struct free_object *obj = slab->free_list;
    slab->free_list = obj->next;
    slab->free_count--;
    cache->allocated_objects++;

    // Gotta move it to another list if this happens
    if(slab->free_count == 0){
        slab_remove_from_list(slab, cache);
        // You get it by now..
        slab->next = cache->full_slabs;
        cache->full_slabs = slab;
    }
    // Was free before but now isn't 
    else if(slab->free_count == cache->objects_per_slab - 1){
        slab_remove_from_list(slab, cache);
        // ?
        slab->next = cache->partial_slabs;
        cache->partial_slabs = slab;
    }

    return (void *)obj;
}

void slab_free(struct slab *slab, void *ptr){
    if(!ptr){
        KERROR("Cannot free a NULL pointer, caller: slab_free\n");
        return;
    }
    
    if(!slab){
        KERROR("Couldn't find slab containing pointer\n");
        return;
    }

    struct slab_cache *cache = slab->cache;
    // We'll get the address of the object we want to free so we can 
    // just cast it without issues  
    struct free_object *obj = (struct free_object *)ptr;
    obj->magic = OBJECT_POISON;
    // Back to the free list you go
    obj->next = slab->free_list;
    slab->free_list = obj;
    slab->free_count++;
    cache->allocated_objects--;
   
    // Was full now is not full
    if(slab->free_count == 1){
        slab_remove_from_list(slab, cache);
        // the hell does this do
        slab->next = cache->partial_slabs;
        cache->partial_slabs = slab;
    } // Had one obj allocated now is empty
    else if(slab->free_count == cache->objects_per_slab){
        slab_remove_from_list(slab, cache);
        slab->next = cache->empty_slabs;
        cache->empty_slabs = slab;
    }
}

struct slab *slab_find_containing(void *ptr) {
    uintptr_t slab_start = (uintptr_t)ptr & ~((2 * PAGE_FRAME_SIZE) - 1);
    struct slab *slab = (struct slab*)slab_start;
    
    if (slab->magic != SLAB_MAGIC)
        return NULL;
    
    // Verify it's actually a valid slab with a cache
    if (!slab->cache) 
        return NULL;
    
    // Pointer is within the objects area of this slab
    char *objects_start = (char *)slab + sizeof(struct slab);
    char *objects_end = objects_start + (slab->cache->objects_per_slab * slab->cache->object_size);
    
    if ((char *)ptr >= objects_start && (char *)ptr < objects_end) 
        return slab;
    
    
    return NULL;
}

bool is_slab_address(void *ptr) {
    return slab_find_containing(ptr) != NULL;
}

void slab_remove_from_list(struct slab *slab, struct slab_cache *cache){
    // What are the odds this is used for linked lists?!?!?!?!?!?!
    struct slab **lists[] = {
        // Partial slabs first as they're most likely what we're removing
        &cache->partial_slabs,
        &cache->full_slabs,
        &cache->empty_slabs
    };

    for(int i = 0; i < 3; i++){
        struct slab **current = lists[i];
        while(*current){
            if(*current == slab){
                *current = slab->next;
                slab->next = NULL;
                return;
            }
            current = &(*current)->next;
        }
    }
}

void *slab_alloc_size(size_t size) {
    // Find appropriate slab cache
    for (size_t i = 0; i < NUM_SLAB_SIZES; i++) {
        if (size <= slab_sizes[i]) {
            return slab_alloc(&slab_caches[i]);
        }
    }
    
    return NULL;
}

void slab_destroy(struct slab *slab){
    if(slab->free_count != slab->cache->objects_per_slab)
        KWARN("Trying to destroy a slab that still has allocated objects\n");


    struct slab_cache *cache = slab->cache;
    slab_remove_from_list(slab, cache);

    cache->total_slabs--;
    cache->total_objects -= cache->objects_per_slab;

    buddy_free_pages(slab->phys_addr, 1);
    kprintf("Destroyed slab for cache (object_size=%lu)\n", cache->object_size);
}

void slab_cache_shrink(struct slab_cache *cache) {
    struct slab **current = &cache->empty_slabs;
    int kept = 0;
    
    // For performance we can keep a few slabs
    while (*current && kept < 2) {
        current = &(*current)->next;
        kept++;
    }
    
    int freed = 0;
    while (*current) {
        struct slab *rms = *current;
        *current = rms->next;
        slab_destroy(rms);
        freed++;
    }
    
    if (freed > 0) {
        kprintf("Kept %d empty slabs and freed %d empty slabs from cache (object_size=%lu)\n", 
              kept, freed, cache->object_size);
    }
}


void slab_print_cache_stats(struct slab_cache *cache) {
    kprintf("Slab Cache Stats (object_size=%lu):\n", cache->object_size);
    kprintf("  Total objects: %lu\n", cache->total_objects);
    kprintf("  Allocated objects: %lu\n", cache->allocated_objects);
    kprintf("  Free objects: %lu\n", cache->total_objects - cache->allocated_objects);
    kprintf("  Total slabs: %lu\n", cache->total_slabs);
    kprintf("  Utilization: %lu%%\n", 
            cache->total_objects ? (cache->allocated_objects * 100) / cache->total_objects : 0);
}

void slab_print_all_stats(void) {
    kprintf("\n=== Slab Allocator Statistics ===\n");
    for (size_t i = 0; i < NUM_SLAB_SIZES; i++) {
        slab_print_cache_stats(&slab_caches[i]);
        kprintf("\n");
    }
}
