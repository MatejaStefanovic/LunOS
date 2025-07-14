#include <kernel/vfs.h>
#include <kernel/pmm.h>
#include <kernel/klogging.h>
#include <kernel/dentry_cache.h>
#include <kernel/compiler.h>
#include <klib/string.h>

static struct filesystem *registered_fs = NULL;
static struct mount_point *mount_table = NULL;
static struct dentry *root_dentry = NULL;

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
    if(d->refcount != 0){
        KWARN("Trying to free a dentry with a refcount greater than 0\n");
        kprintf("Dentry name: %s", d->name); 
    }

    if(d->inode)
        d->inode->refcount--;
    
    dcache_remove(d);
  
    kfree(d->name);
    kfree(d);
}

struct dentry *path_walk(const char *path, struct path *p){
    if(!path)
        return NULL;

    if(strcmp(path, "/") == 0){
        root_dentry->refcount++;
        return root_dentry;
    }

    struct dentry *parent = root_dentry;
    
    if(p && p->cwd)
        parent = p->cwd;

    parent->refcount++;
    
    // We don't want the first / in the path
    // so skip it
    if(path[0] == '/')
        path++;

    char *path_copy = kstrndup(path, strlen(path));
    // TODO: this saveptr needs to be CPU specific later
    char *saveptr;
    char *token = kstrtok_r(path_copy, "/", &saveptr);

    while(token){
        // Empty token, basically if the path is something /home///a//b
        // we'll get empty tokens so skip those
        if(!*token){
            token = kstrtok_r(NULL, "/", &saveptr);
            continue;
        }

        struct dentry *current = dcache_lookup(parent, token); 

        // Not in cache
        if(!current){
            current = parent->inode->i_ops->lookup(parent->inode, token);

            // Wasn't able to be looked up
            if(!current){
                parent->refcount--;
                kfree(path_copy);
                return NULL;
            }

            dcache_add(current);
        }
        
        // Last token means it is a file so we can return
        if(saveptr && *saveptr == '\0'){
            parent->refcount--; 
            current->refcount++;
            kfree(path_copy);
            return current; 
        }  
        
        // Every token except that last one must be a dir
        if(!IS_DIRECTORY(current->inode->mode)){
            parent->refcount--;
            kfree(path_copy);
            return NULL;
        }

        parent->refcount--;
        current->refcount++;
        parent = current;
        token = kstrtok_r(NULL, "/", &saveptr);
    }
    
    kfree(path_copy);
    return parent;
}

struct inode *alloc_inode(struct super_block *sb){
    if(!sb)
        return NULL;

    struct inode *ind = kmalloc(sizeof(*ind));
    if(!ind)
        return NULL;

    ind->sb = sb;
    ind->refcount = 1;
    
    list_init(&ind->sb_inode_list);
    list_add_tail(&ind->sb_inode_list, &sb->sb_inode);
    
    return ind;
}

void free_inode(struct inode *ind){
    if(!ind)
        return;

    // TODO: adjust refcounts of things that will use inodes 
    // right now I don't know which things will use inodes exactly
    if(ind->refcount > 0)
        KWARN("Trying to free an inode with a refcount greater than 0\n");
    
    // Later this might change into a filesystem specific cleanup method Idk 
    if(ind->i_ops)
        kfree(ind->i_ops);
    
    if(ind->f_ops)
        kfree(ind->f_ops);
    
    if(ind->private_data)
        kfree(ind->private_data);

    list_del(&ind->sb_inode_list);
    kfree(ind);
}

int register_filesystem(struct filesystem *fs){
    if(!fs || !fs->name)
        return VFS_EINVAL;

    fs->next = registered_fs;
    registered_fs = fs;

    return VFS_OK;
}

struct filesystem *find_filesystem(const char *name){
    if(!name)
        return NULL;

    struct filesystem *fs = registered_fs;
    while(fs){
        if(strcmp(fs->name, name) == 0)
            return fs;
        fs = fs->next;
    }
}

