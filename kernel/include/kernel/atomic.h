#ifndef __KERNEL_ATOMIC_H
#define __KERNEL_ATOMIC_H

#define LOCK_PREFIX "\n\tlock; "

// You shall not pass!
#define memory_barrier()    __asm__ __volatile__("mfence" ::: "memory")
#define read_barrier()      __asm__ __volatile__("lfence" ::: "memory")
#define write_barrier()     __asm__ __volatile__("sfence" ::: "memory")
#define compiler_barrier()  __asm__ __volatile__("" ::: "memory")

#define cpu_pause()         __asm__ __volatile__("pause" ::: "memory")

// I don't think I'll support other archs so this is fine for now
typedef struct {
    volatile int value;
} atomic;

typedef struct {
    volatile long value;
} atomic64; 

// Compile time inits
#define ATOMIC_INIT(val)    { .value = (val) }
#define ATOMIC64_INIT(val)  { .value = (val) }

// THIS PART IS NOT ATOMIC YET maybe will
// make it that way in the future if I need it

// Runtime inits
static inline void atomic_init(atomic *v, int val){
    v->value = val;
}

static inline void atomic64_init(atomic64 *v, long val){
    v->value = val;
}

// Basic read/write operations
static inline int atomic_read(const atomic *v){
    return v->value;
}

static inline long atomic64_read(const atomic64 *v){
    return v->value;
}

static inline void atomic_set(atomic *v, int val){
    v->value = val;
}

static inline void atomic64_set(atomic64 *v, long val){
    v->value = val;
}

static inline void atomic_add(int val, atomic *v){
    __asm__ __volatile__(LOCK_PREFIX "addl %1, %0"
                         : "+m" (v->value)
                         : "ir" (val)
                         : "memory");
}

static inline void atomic64_add(long val, atomic64 *v){
    __asm__ __volatile__(LOCK_PREFIX "addq %1, %0"
                         : "+m" (v->value)
                         : "er" (val)
                         : "memory");
}

static inline void atomic_sub(int val, atomic *v){
    __asm__ __volatile__(LOCK_PREFIX "subl %1, %0"
                         : "+m" (v->value)
                         : "ir" (val)
                         : "memory");
}

static inline void atomic64_sub(long val, atomic64 *v){
    __asm__ __volatile__(LOCK_PREFIX "subq %1, %0"
                         : "+m" (v->value)
                         : "er" (val)
                         : "memory");
}

static inline void atomic_inc(atomic *v){
    __asm__ __volatile__(LOCK_PREFIX "incl %0"
                         : "+m" (v->value)
                         :
                         : "memory");
}

static inline void atomic64_inc(atomic64 *v){
    __asm__ __volatile__(LOCK_PREFIX "incq %0"
                         : "+m" (v->value)
                         :
                         : "memory");
}

static inline void atomic_dec(atomic *v){
    __asm__ __volatile__(LOCK_PREFIX "decl %0"
                         : "+m" (v->value)
                         :
                         : "memory");
}

static inline void atomic64_dec(atomic64 *v){
    __asm__ __volatile__(LOCK_PREFIX "decq %0"
                         : "+m" (v->value)
                         :
                         : "memory");
}

// Atomic add/sub and return result
// Note: we return result+val instead of v->value (despite them being the same)
// as v->value would require an additional instruction to read the memory 
// and that would break atomicity 
static inline int atomic_add_return(int val, atomic *v){
    int result = val;
    __asm__ __volatile__(LOCK_PREFIX "xaddl %0, %1"
                         : "+r" (result), "+m" (v->value)
                         :
                         : "memory");
    return result + val;
}

static inline long atomic64_add_return(long val, atomic64 *v){
    long result = val;
    __asm__ __volatile__(LOCK_PREFIX "xaddq %0, %1"
                         : "+r" (result), "+m" (v->value)
                         :
                         : "memory");
    return result + val;
}

static inline int atomic_sub_return(int val, atomic *v){
    return atomic_add_return(-val, v);
}

static inline long atomic64_sub_return(long val, atomic64 *v){
    return atomic64_add_return(-val, v);
}

// Atomic increment/decrement and return result
static inline int atomic_inc_return(atomic *v){
    return atomic_add_return(1, v);
}

static inline long atomic64_inc_return(atomic64 *v){
    return atomic64_add_return(1, v);
}

static inline int atomic_dec_return(atomic *v){
    return atomic_add_return(-1, v);
}

static inline long atomic64_dec_return(atomic64 *v){
    return atomic64_add_return(-1, v);
}

// Test and return operations
static inline int atomic_inc_and_test(atomic *v){
    char result;
    __asm__ __volatile__(LOCK_PREFIX "incl %0; setz %1"
                         : "+m" (v->value), "=qm" (result)
                         :
                         : "memory");
    return result;
}

static inline int atomic_dec_and_test(atomic *v){
    char result;
    __asm__ __volatile__(LOCK_PREFIX "decl %0; setz %1"
                         : "+m" (v->value), "=qm" (result)
                         :
                         : "memory");
    return result;
}

// Basically just a check to see if sum of things is negative
static inline int atomic_add_negative(int val, atomic *v){
    char result;
    __asm__ __volatile__(LOCK_PREFIX "addl %2, %0; sets %1"
                         : "+m" (v->value), "=qm" (result)
                         : "ir" (val)
                         : "memory");
    return result;
}

// Atomic exchange operations
// Note: These are implicitly atomic and do not require the lock prefix
static inline int atomic_xchg(atomic *v, int new_val){
    int old_val;
    __asm__ __volatile__("xchgl %0, %1"
                         : "=r" (old_val), "+m" (v->value)
                         : "0" (new_val)
                         : "memory");
    return old_val;
}

static inline long atomic64_xchg(atomic64 *v, long new_val){
    long old_val;
    __asm__ __volatile__("xchgq %0, %1"
                         : "=r" (old_val), "+m" (v->value)
                         : "0" (new_val)
                         : "memory");
    return old_val;
}

// Compare and swap operations **the infamous CAS**
static inline int atomic_cmpxchg(atomic *v, int old_val, int new_val){
    int prev;
    __asm__ __volatile__(LOCK_PREFIX "cmpxchgl %1, %2"
                         : "=a" (prev)
                         : "r" (new_val), "m" (v->value), "0" (old_val)
                         : "memory");
    return prev;
}

static inline long atomic64_cmpxchg(atomic64 *v, long old_val, long new_val){
    long prev;
    __asm__ __volatile__(LOCK_PREFIX "cmpxchgq %1, %2"
                         : "=a" (prev)
                         : "r" (new_val), "m" (v->value), "0" (old_val)
                         : "memory");
    return prev;
}

// Boolean compare and swap - returns 1 if successful, 0 if failed
static inline int atomic_try_cmpxchg(atomic *v, int *old_val, int new_val){
    char success;
    int prev = *old_val;
    __asm__ __volatile__(LOCK_PREFIX "cmpxchgl %3, %1; setz %0"
                         : "=a" (success), "+m" (v->value), "+a" (prev)
                         : "r" (new_val)
                         : "memory");
    *old_val = prev;
    return success;
}

static inline int atomic64_try_cmpxchg(atomic64 *v, long *old_val, long new_val){
    char success;
    long prev = *old_val;
    __asm__ __volatile__(LOCK_PREFIX "cmpxchgq %3, %1; setz %0"
                         : "=a" (success), "+m" (v->value), "+a" (prev)
                         : "r" (new_val)
                         : "memory");
    *old_val = prev;
    return success;
}

// Bit operations
static inline void atomic_set_bit(int bit, volatile unsigned long *addr){
    __asm__ __volatile__(LOCK_PREFIX "btsq %1, %0"
                         : "+m" (*addr)
                         : "Ir" (bit)
                         : "memory");
}

static inline void atomic_clear_bit(int bit, volatile unsigned long *addr){
    __asm__ __volatile__(LOCK_PREFIX "btrq %1, %0"
                         : "+m" (*addr)
                         : "Ir" (bit)
                         : "memory");
}

static inline void atomic_change_bit(int bit, volatile unsigned long *addr){
    __asm__ __volatile__(LOCK_PREFIX "btcq %1, %0"
                         : "+m" (*addr)
                         : "Ir" (bit)
                         : "memory");
}

static inline int atomic_test_and_set_bit(int bit, volatile unsigned long *addr){
    char old_bit;
    __asm__ __volatile__(LOCK_PREFIX "btsq %2, %0; setc %1"
                         : "+m" (*addr), "=qm" (old_bit)
                         : "Ir" (bit)
                         : "memory");
    return old_bit;
}

static inline int atomic_test_and_clear_bit(int bit, volatile unsigned long *addr){
    char old_bit;
    __asm__ __volatile__(LOCK_PREFIX "btrq %2, %0; setc %1"
                         : "+m" (*addr), "=qm" (old_bit)
                         : "Ir" (bit)
                         : "memory");
    return old_bit;
}

static inline int atomic_test_and_change_bit(int bit, volatile unsigned long *addr){
    char old_bit;
    __asm__ __volatile__(LOCK_PREFIX "btcq %2, %0; setc %1"
                         : "+m" (*addr), "=qm" (old_bit)
                         : "Ir" (bit)
                         : "memory");
    return old_bit;
}

// Non-atomic bit test
static inline int test_bit(int bit, const volatile unsigned long *addr){
    return (addr[bit >> 6] >> (bit & 63)) & 1;
}

// Utility macros for common patterns
#define atomic_read_acquire(v)      ({ int __val = atomic_read(v); read_barrier(); __val; })
#define atomic_set_release(v, val)  do { write_barrier(); atomic_set(v, val); } while(0)

#define atomic64_read_acquire(v)      ({ long __val = atomic64_read(v); read_barrier(); __val; })
#define atomic64_set_release(v, val)  do { write_barrier(); atomic64_set(v, val); } while(0)

#endif
