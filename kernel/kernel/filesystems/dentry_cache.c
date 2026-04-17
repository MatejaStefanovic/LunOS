#include <kernel/dentry_cache.h>
#include <kernel/vfs.h>
#include <kernel/klogging.h>
#include <kernel/compiler.h>
#include <klib/string.h>

#define DENTRY_HASH_SIZE 256

static struct dentry *dentry_hash_table[DENTRY_HASH_SIZE];

// Modified Jenkins one time hash
static size_t hash_function(uint64_t parent_inode_num, const char *key, size_t length){
    size_t hash = 0;

    // Hash parent's inode number for special case
    // where both dentries have the same name
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        hash += (parent_inode_num >> (i * 8)) & 0xFF;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    // Hash the dentry name
    for (size_t i = 0; i < length; i++) {
        hash += (unsigned char)key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

static int dentry_ht_bucket(struct dentry *parent, const char *name) {
    uint64_t parent_inode_num; 
    if(_likely(parent))
        parent_inode_num = parent->inode->inode_number;
    else
        parent_inode_num = 0;
    size_t hash = hash_function(parent_inode_num, name, strlen(name)); 
    return hash % DENTRY_HASH_SIZE; 
}

void dcache_init(void){
    for(int i = 0; i < DENTRY_HASH_SIZE; i++){
        dentry_hash_table[i] = NULL;
    }
}

void dcache_add(struct dentry *entry){
    if(!entry)
        return;
    
    size_t bucket = dentry_ht_bucket(entry->parent, entry->name);
    // bucket is a linked list and this is just link at 
    // the head
    entry->d_hash_next = dentry_hash_table[bucket];
    dentry_hash_table[bucket] = entry;
}

void dcache_remove(struct dentry *entry){
    if(!entry)
        return;

    size_t bucket = dentry_ht_bucket(entry->parent, entry->name);

    struct dentry **current = &dentry_hash_table[bucket];

    while(*current){
        if(strcmp((*current)->name, entry->name) == 0){
            *current = (*current)->d_hash_next;
            return;
        }
        current = &(*current)->d_hash_next;
    }
}

struct dentry *dcache_lookup(struct dentry *parent, const char *name){
    if(!parent || !name){
        KWARN("Cannot lookup null entry");
        return NULL;
    }
    
    size_t bucket = dentry_ht_bucket(parent, name);
    struct dentry *current = dentry_hash_table[bucket];

    while(current){
        if(strcmp(current->name, name) == 0)
            return current;
        current = current->d_hash_next;
    }

    return NULL;
}
