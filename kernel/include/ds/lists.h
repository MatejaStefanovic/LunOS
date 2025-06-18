#ifndef __KERNEL_DS_LIST_H
#define __KERNEL_DS_LIST_H

#include <stddef.h>

struct list_node {
    struct list_node *next; 
    struct list_node *prev;
};

static inline void list_init(struct list_node *list){
    list->next = list;
    list->prev = list;
}

static inline void list_add(struct list_node *new_node, struct list_node *prev,
                                                        struct list_node *next){
    next->prev = new_node;
    new_node->next = next;
    new_node->prev = prev;
    prev->next = new_node;
}

static inline void list_add_head(struct list_node *new_node, struct list_node *head){
    list_add(new_node, head, head->next);
}

static inline void list_add_tail(struct list_node *new_node, struct list_node *head){
    list_add(new_node, head->prev, head);
}

static inline void list_del(struct list_node *entry){
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    entry->next = NULL;
    entry->prev = NULL;
}

static inline void list_replace(struct list_node *old, struct list_node *new_node){
    new_node->next = old->next;
    new_node->next->prev = new_node;
    new_node->prev = old->prev;
    new_node->prev->next = new_node;
}

static inline int list_empty(const struct list_node *head){
    return head->next == head;
}

// Get the struct from the entry
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#endif
