#ifndef __KERNEL_D_CACHE_H
#define __KERNEL_D_CACHE_H

struct dentry;

void dcache_init(void);
void dcache_add(struct dentry *d);
void dcache_remove(struct dentry *d);
struct dentry *dcache_lookup(struct dentry *parent, const char *name);

#endif
