#ifndef __KERNEL_VFS_H
#define __KERNEL_VFS_H

#include <stddef.h>
#include <string.h>
#include <ds/lists.h>

// signed size_t so we can return -1 on err
// (0 is valid because we can read 0 bytes)
typedef long ssize_t;
typedef long off_t;

// It is in octal because UNIX rwx permissions are in octal
#define INODE_FILE_TYPE_MASK    00170000  // bitmask for the file type bitfields
#define INODE_FILE_SOCKET       0140000   // socket
#define INODE_FILE_LINK         0120000   // symbolic link
#define INODE_FILE_REGULAR_FILE 0100000   // regular file
#define INODE_FILE_BLOCK_DEVICE 0060000   // block device
#define INODE_FILE_DIRECTORY    0040000   // directory
#define INODE_FILE_CHAR_DEVICE  0020000   // character device
#define INODE_FILE_FIFO         0010000   // FIFO/pipe

// Check the file type macros
#define IS_LINK(type_mask)  (((type_mask) & INODE_FILE_TYPE_MASK) == INODE_FILE_LINK)
#define IS_REGULAR_FILE(type_mask)  (((type_mask) & INODE_FILE_TYPE_MASK) == INODE_FILE_REGULAR_FILE)
#define IS_DIRECTORY(type_mask)  (((type_mask) & INODE_FILE_TYPE_MASK) == INODE_FILE_DIRECTORY)
#define IS_CHAR_DEVICE(type_mask)  (((type_mask) & INODE_FILE_TYPE_MASK) == INODE_FILE_CHAR_DEVICE)
#define IS_BLOCK_DEVICE(type_mask)  (((type_mask) & INODE_FILE_TYPE_MASK) == INODE_FILE_BLOCK_DEVICE)
#define IS_FIFO(type_mask) (((type_mask) & INODE_FILE_TYPE_MASK) == INODE_FILE_FIFO)
#define IS_SOCKET(type_mask) (((type_mask) & INODE_FILE_TYPE_MASK) == INODE_FILE_SOCKET)

// File access modes
#define READ_ONLY   0
#define WRITE_ONLY  1
#define READ_WRITE  2

struct file;
struct super_block;
struct inode;

struct file_operations {
    ssize_t (*read)(struct file *file, char *buf, size_t count, off_t offset);
    ssize_t (*write)(struct file *file, const char *buf, size_t count, off_t offset);
    int (*open)(struct inode *inode, struct file *file);
    int (*close)(struct inode *inode, struct file *file);
    int (*flush)(struct file *file);
    int (*fsync)(struct file *file);
    off_t (*lseek)(struct file *file, off_t offset, int whence);
};

struct inode_operations {
    struct inode *(*lookup)(struct inode *dir, const char *name);
    int (*create)(struct inode *dir, const char *name, int mode);
    int (*mkdir)(struct inode *dir, const char *name, int mode);
    int (*unlink)(struct inode *dir, const char *name);
    int (*rmdir)(struct inode *dir, const char *name);
    int (*rename)(struct inode *old_dir, const char *old_name, struct inode *new_dir, const char *new_name);
    int (*link)(struct inode *old_inode, struct inode *dir, const char *name);
    int (*symlink)(struct inode *dir, const char *name, const char *target);
};

struct superblock_operations {
    struct inode *(*alloc_inode)(struct super_block sb);
    void (*free_inode)(struct inode *inode);
    int (*write_inode)(struct inode *inode);
    void (*put_super)(struct super_block *sb);
};

struct inode {
    unsigned long inode_number;
    struct super_block *sb;
    struct inode_operations *i_ops;
    struct file_operations *f_ops;
    unsigned int mode;      // Type and permissions
    unsigned int uid, gid;  // Won't have users but this is needed for compatibility with Linux
    unsigned long size;
    int ref_count;
    struct list_node sb_inode_list; // Node in superblock's inode list 
    // Data used depending on the instance of the inode
    // Different drivers may require different data 
    // and that is stored here
    void *private_data;
};

struct dentry {
    char *name;
    struct inode *inode;
    struct dentry *parent;
    struct list_node children;
    struct list_node sibling;
    int ref_count;
    int flags;
    struct dentry *d_hash; // for hash table linking
};

// Superblock is like a header for a file system instance
struct super_block {
    struct filesystem *fs;
    struct superblock_operations *s_ops;
    struct inode *root_inode;
    struct list_node sb_inode;  // All inodes for this superblock
    unsigned long block_size;
    unsigned long magic;        // Filesystem magic number
    // Same story as before, private data depending on
    // which file system this superblock represents
    void *private_data;
};

struct filesystem {
    const char *name;
    int (*mount)(struct super_block *sb, void *data);
    int (*unmount)(struct super_block *sb);
    struct filesystem *next; // Linked list of all filesystems
};

struct mount_point {
    char *path;
    struct super_block *sb;
    struct mount_point *next;
};

// Error codes
#define VFS_OK 0
#define VFS_ENOENT -1
#define VFS_ENOMEM -2
#define VFS_EIVAL -3
#define VFS_ENOTDIR -4

struct dentry *alloc_dentry(const char *name, struct inode *inode);
void free_dentry(struct dentry *dentry);
struct dentry *path_walk(const char *path);

struct inode *alloc_inode(struct super_block *sb);
void free_inode(struct inode *inode);

int register_filesystem(struct filesystem *fs);
struct filesystem *find_filesystem(const char *name);

// File operations
int vfs_open(const char *path, int flags, struct file **file_out);
int vfs_close(struct file *file);
ssize_t vfs_read(struct file *file, char *buf, size_t count); 
ssize_t vfs_write(struct file *file, const char *buf, size_t count);
int vfs_lseek(struct file *file, off_t offset, int whence);

// Directory operations  
int vfs_mkdir(const char *path, int mode);
int vfs_rmdir(const char *path);
int vfs_unlink(const char *path);
int vfs_create(const char *path, int mode);
int vfs_rename(const char *old_path, const char *new_path);

// Filesystem operations 
int vfs_mount(const char *fs_name, const char *device_path, const char *mount_path);
int vfs_unmount(const char *mount_path);

void vfs_debug_print_mounts(void);

#endif
