#ifndef __KERNEL_COMPILER_MACROS_H
#define __KERNEL_COMPILER_MACROS_H

#define _packed         __attribute__((packed))
#define _aligned(x)     __attribute__((aligned(x)))
#define _page_aligned   _aligned(PAGE_SIZE)
#define _print_fmt(fmt_idx, args_idx) __attribute__((format(printf, fmt_idx, args_idx)))

// Branch prediction hints
#define _likely(x)      __builtin_expect(!!(x), 1)
#define _unlikely(x)    __builtin_expect(!!(x), 0)

#endif
