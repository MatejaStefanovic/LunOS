#ifndef __KERNEL_SPINLOCK_H
#define __KERNEL_SPINLOCK_H

#include <kernel/atomic.h>
#include <stdint.h>

typedef uint64_t int_flags;

static inline int_flags save_and_disable_interrupts(void){
    int_flags flags;
    __asm__ __volatile__("pushfq; popq %0; cli" 
                         : "=rm" (flags) 
                         : // no input
                         : "memory");
    return flags;
}

static inline void restore_interrupts(int_flags flags){
    __asm__ __volatile__("pushq %0; popfq" 
                         : // no output
                         : "rm" (flags) 
                         : "memory", "cc");
}

typedef struct {
    atomic lock;
} spinlock;

// One lock, 1 bit for writer (bit 31) and 30 bits for readers 
typedef struct {
    atomic lock;      
} rwlock;

#define SPINLOCK_INIT           { .lock = ATOMIC_INIT(0) }
#define RWLOCK_INIT             { .lock = ATOMIC_INIT(0) }

static inline void spinlock_init(spinlock *lock){
    atomic_set(&lock->lock, 0);
}

static inline void rwlock_init(rwlock *lock){
    atomic_set(&lock->lock, 0);
}

// Check if spinlock is locked (non-atomic check for debugging)
static inline int spinlock_is_locked(spinlock *lock){
    return atomic_read(&lock->lock) != 0;
}

static inline void spinlock_lock(spinlock *lock){
    // Try to get the lock if possible
    while (atomic_xchg(&lock->lock, 1) != 0) {
        // If we didn't get it we spin infinitely but while checking with 
        // atomic_read as that instruction is less expensive than xchg 
        while (atomic_read(&lock->lock) != 0) {
            cpu_pause();  
        }
    }
}

static inline int spinlockrylock(spinlock *lock){
    return atomic_xchg(&lock->lock, 1) == 0;
}

static inline void spinlock_unlock(spinlock *lock){
    memory_barrier();
    atomic_set(&lock->lock, 0);
}

static inline void acquire_locks_ordered(spinlock *lock_a, spinlock *lock_b) {
    spinlock *first, *second;

    // We use memory addresses to create ordering
    if ((uintptr_t)lock_a < (uintptr_t)lock_b) {
        first = lock_a;   // lock_a has lower memory address
        second = lock_b;
    } else {
        first = lock_b;   // lock_b has lower memory address  
        second = lock_a;
    }
    
    spinlock_lock(first);
    spinlock_lock(second);
}

// Why do we need this?
// Scenario: One of our CPUs (let's go with CPU 0) does something like:
// kprintf, this of course needs to be under lock as we don't want
// additional CPUs to print at the same time over each other
// what can happen is that some interrupt may activate for our CPU 0
// and if that interrupt tries to use kprintf (like a keyboard IRQ might do)
// from that interrupt our kprintf will try to acquire the lock but the lock is 
// already taken (from our first kprintf) and since we never return from the interrupt
// to release the lock it will cause a deadlock for CPU 0
static inline void spinlock_lock_intsave(spinlock *lock, int_flags *flags){
    *flags = save_and_disable_interrupts();
    spinlock_lock(lock);
}

static inline void spinlock_unlock_intrestore(spinlock *lock, int_flags flags){
    spinlock_unlock(lock);
    restore_interrupts(flags);
}

/* ======= READ WRITE SPINLOCK ======= */

// 30 bits for readers, 1 bit for writer 
#define RWLOCK_WRITER_BIT       31
#define RWLOCK_WRITER_MASK      (1U << RWLOCK_WRITER_BIT)
#define RWLOCK_READER_MASK      (RWLOCK_WRITER_MASK - 1)

static inline void rwlock_read_lock(rwlock *lock){
    int old_val, new_val;
    
    do {
        old_val = atomic_read(&lock->lock);
        
        // Wait if a writer has the lock
        while (old_val & RWLOCK_WRITER_MASK) {
            cpu_pause();
            old_val = atomic_read(&lock->lock);
        }
        
        // Try to increment reader count
        new_val = old_val + 1;
        
        // Make sure we don't overflow into writer bit
        if ((new_val & RWLOCK_READER_MASK) == 0) {
            continue;  // Overflow, try again
        }
        
    } while (atomic_cmpxchg(&lock->lock, old_val, new_val) != old_val);
}

static inline int rwlock_read_trylock(rwlock *lock){
    int old_val = atomic_read(&lock->lock);
    
    // Can't acquire if writer has lock
    if (old_val & RWLOCK_WRITER_MASK) {
        return 0;
    }
    
    int new_val = old_val + 1;
    
    // Check for overflow
    if ((new_val & RWLOCK_READER_MASK) == 0) {
        return 0;
    }
    
    return atomic_cmpxchg(&lock->lock, old_val, new_val) == old_val;
}

static inline void rwlock_read_unlock(rwlock *lock){
    atomic_dec(&lock->lock);
}

static inline void rwlock_write_lock(rwlock *lock){
    // Try to set the writer bit
    while (atomic_cmpxchg(&lock->lock, 0, RWLOCK_WRITER_MASK) != 0) {
        // Wait for all readers and writers to finish
        while (atomic_read(&lock->lock) != 0) {
            cpu_pause();
        }
    }
}

static inline int rwlock_write_trylock(rwlock *lock){
    return atomic_cmpxchg(&lock->lock, 0, RWLOCK_WRITER_MASK) == 0;
}

static inline void rwlock_write_unlock(rwlock *lock){
    memory_barrier();
    atomic_set(&lock->lock, 0);
}

/* ======= TICKET SPINLOCK ======= */
typedef struct {
    atomic next_ticket;
    atomic serving_ticket;
} ticket_spinlock;

#define TICKET_SPINLOCK_INIT    { .next_ticket = ATOMIC_INIT(0), .serving_ticket = ATOMIC_INIT(0) }

static inline void ticket_spinlock_init(ticket_spinlock *lock){
    atomic_set(&lock->next_ticket, 0);
    atomic_set(&lock->serving_ticket, 0);
}

static inline void ticket_spinlock_lock(ticket_spinlock *lock){
    int my_ticket = atomic_add_return(1, &lock->next_ticket) - 1;
    
    while (atomic_read(&lock->serving_ticket) != my_ticket) {
        cpu_pause();
    }
}

static inline int ticket_spinlockrylock(ticket_spinlock *lock){
    int serving = atomic_read(&lock->serving_ticket);
    int next = atomic_read(&lock->next_ticket);
    
    if (serving != next) {
        return 0;  // Someone else has a ticket
    }
    
    return atomic_cmpxchg(&lock->next_ticket, next, next + 1) == next;
}

static inline void ticket_spinlock_unlock(ticket_spinlock *lock){
    memory_barrier();
    atomic_inc(&lock->serving_ticket);
}

#define DEFINE_SPINLOCK(name)           spinlock name = SPINLOCK_INIT
#define DEFINE_RWLOCK(name)             rwlock name = RWLOCK_INIT
#define DEFINE_TICKET_SPINLOCK(name)    ticket_spinlock name = TICKET_SPINLOCK_INIT

#endif
