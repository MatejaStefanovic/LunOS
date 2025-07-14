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

// We can either take in the absolute path or the relative path from
// the current working directory
struct dentry *path_walk(const char *abs_path, struct path *wd_path){
    if(!abs_path)
        return NULL;

    if(strcmp(abs_path, "/") == 0){
        root_dentry->refcount++;
        return root_dentry;
    }

    struct dentry *parent = root_dentry;
    
    if(wd_path && wd_path->dentry)
        parent = wd_path->dentry;

    // Use case: cd ..
    if (strcmp(abs_path, "..") == 0) {
        struct dentry *prev_dir = parent->parent ? parent->parent : parent;
        prev_dir->refcount++; 
        return prev_dir;
    }
    
    if(strcmp(abs_path, ".") == 0){
        parent->refcount++;
        return parent;
    }    
   
    // Skip the leading /
    if(abs_path[0] == '/')
        abs_path++;
    
    parent->refcount++; 
    char *path_copy = kstrndup(abs_path, strlen(abs_path));
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

        // Use case: ./a.out
        if (strcmp(token, ".") == 0) {
            token = kstrtok_r(NULL, "/", &saveptr);
            continue;
        }

        // Use case: ../here/there 
        if (strcmp(token, "..") == 0) {
            struct dentry *prev = parent->parent ? parent->parent : parent;
            prev->refcount++;
            parent->refcount--;
            parent = prev;

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
    
    return NULL;
}

int vfs_open(const char *abs_path, struct path *wd, int flags, struct file **file_out){
    if(!abs_path || !file_out)
        return  VFS_EINVAL;

    struct dentry *dirent = path_walk(abs_path, wd);
    if(!dirent)
        return VFS_ENOENT;

    if(!IS_DIRECTORY(dirent->inode->mode))
        return VFS_EISDIR;       

    struct file *f = kmalloc(sizeof(*f));
    if(!f)
        return VFS_ENOMEM;

    f->fpath.dentry = dirent;
    f->inode = dirent->inode;
    f->offset = 0;
    f->flags = flags;
    f->mode = 0;  // FS sets this 
    f->private_data = NULL;
    f->f_ops = dirent->inode->f_ops;
    
    dirent->refcount++;
    f->inode->refcount++;

    if(f->f_ops && f->f_ops->open){
        int ret = f->f_ops->open(f->inode, f);
        if(ret != VFS_OK){
            dirent->refcount--;
            f->inode->refcount--;
            kfree(f);
            return ret;
        }
    }

    *file_out = f;
    return VFS_OK;
}

int vfs_close(struct file *f){
    if(!f)
        return VFS_EINVAL;
    
    int ret = VFS_OK;

    // Some filesystems do not have close operation
    if(f->f_ops && f->f_ops->close)
        ret = f->f_ops->close(f->inode, f);
    
    f->fpath.dentry->refcount--;
    f->inode->refcount--;

    kfree(f);
    return ret;
}

ssize_t vfs_read(struct file *f, char *buf, size_t count){
    if(!f || !buf || !f->f_ops || !f->f_ops->read)
        return -VFS_EINVAL;
    
    ssize_t amount_read = f->f_ops->read(f, buf, count, f->offset);
    return amount_read;
}

ssize_t vfs_write(struct file *f, const char *buf, size_t count){
    if(!f || !buf || !f->f_ops || !f->f_ops->write)
        return -VFS_EINVAL;

    ssize_t amount_written = f->f_ops->write(f, buf, count, f->offset);
    return amount_written;
}

off_t vfs_lseek(struct file *f, off_t offset, int whence){ 
    if(!f || !f->f_ops || !f->f_ops->lseek)
        return -VFS_EINVAL;
    
    off_t new_offset = f->f_ops->lseek(f, offset, whence);  
    return new_offset;
}




void vfs_debug_print_mounts(void) {
    struct mount_point *mp = mount_table;
    kprintf("Mount table:\n");
    while (mp) {
        kprintf("  %s -> %s\n", mp->path, mp->sb->fs->name);
        mp = mp->next;
    }
}
