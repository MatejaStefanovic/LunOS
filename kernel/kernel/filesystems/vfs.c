#include <kernel/vfs.h>
#include <kernel/pmm.h>
#include <kernel/klogging.h>
#include <kernel/dentry_cache.h>
#include <kernel/compiler.h>
#include <klib/string.h>

struct dentry *alloc_dentry(struct dentry *parent, const char *name){
    struct dentry *d = kmalloc(sizeof(*d));
    if(!d)
        return NULL;

    d->name = kstrndup(name, DENTRY_NAME_MAX_LENGTH);
    if(!d->name){ 
        kfree(d); // Free what we already allocated
        return NULL;
    }

    list_init(&d->children);
    list_init(&d->siblings);
    d->parent = parent;
    
    // root directory has no parent hence the check
    if(_likely(parent)) 
        list_add_tail(&d->siblings, &parent->children);

    d->refcount = 1; 
    d->flags = 0;
    
    dcache_add(d);

    return d;
}

void instantiate_dentry(struct dentry *entry, struct inode *i_node){
    if(!i_node || !entry){
        KWARN("Given inode or dentry were NULL, something went wrong\n");
        return;
    }
    entry->inode = i_node;
    i_node->refcount++; 
}

void free_dentry(struct dentry *d){
    if(d->refcount != 0)
        KWARN("Dentry with a refcount higher than 0 was asked to be freed\n");

    if(d->inode)
        d->inode->refcount--;
    
    dcache_remove(d);
  
    kfree(d->name);
    kfree(d);
}
