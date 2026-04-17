# x86-64 Kernel

> A custom 64-bit operating system kernel for the x86 architecture

![Architecture](https://img.shields.io/badge/arch-x86--64-blue)
![SMP](https://img.shields.io/badge/SMP-enabled-green)
![Scheduler](https://img.shields.io/badge/scheduler-preemptive-yellow)
![Language](https://img.shields.io/badge/language-C%20%2F%20Assembly-lightgrey)
![License](https://img.shields.io/badge/license-GPL%20v2-lightgrey)

---

## Overview

A from-scratch 64-bit kernel targeting the x86 architecture. Built to explore low-level systems programming — from physical page allocation all the way up to symmetric multiprocessing across CPU cores. No educational framework, no shortcuts.

---

## Features

### Physical Memory Manager
Hybrid **buddy allocator** + **slab allocator** for low-fragmentation allocation. The buddy system handles large power-of-two block requests while the slab allocator efficiently manages fixed-size kernel objects, minimizing internal fragmentation.

### Virtual Memory Manager
4-level paging with per-process address space isolation, enabling safe multitasking and a clean kernel/user memory separation. Each process gets its own page table hierarchy.

### Symmetric Multiprocessing (SMP)
Full SMP support using Local APIC timers to drive preemptive task switching independently on each CPU core.

### Scheduler & Load Balancer
A preemptive scheduler with a load balancer for fair task distribution across cores. Minimizes idle time and maximizes throughput by redistributing tasks when core utilization is uneven.

### Virtual File System (VFS)
A VFS abstraction layer providing a unified interface for future filesystem implementations. Designed to support multiple concrete filesystem drivers (ext2, FAT32, etc.) without changes to upper-layer kernel code.

---

## Architecture

| Subsystem | Implementation | Notes |
|---|---|---|
| Physical allocator | Buddy + Slab | Buddy for large blocks, slab for fixed-size kernel objects |
| Virtual memory | 4-level paging | Per-process page tables, modified demand paging |
| SMP | APIC + IPI | Local APIC timers drive preemption per-core |
| Scheduler | Preemptive + load balancer | Work stealing / fair distribution across cores |
| Filesystem | VFS layer | Abstract interface — concrete drivers TBD |

---

## Building & Running

### Prerequisites

- `as` — (GNU Assembler / GAS) - assembler, used **binutils 2.44**
- `gcc` cross-compiler targeting `x86_64-elf`, used **GCC 15.1.0**
- `qemu-system-x86_64` — for emulation
- `make`

### Build

```bash
git clone https://github.com/MatejaStefanovic/LunOS
cd kernel
./build.sh          # compile the kernel
./run.sh            # build and immediately run in QEMU
./qemu.sh           # build and immediately run in QEMU
./clean.sh          # remove build artifacts
./sparse.sh         # semantic checker for C programs
```

> Tested on QEMU and real hardware. The bare-metal boot via Limine protocol is supported but not guaranteed on all hardware.

---

## Roadmap

- [x] Physical memory manager (buddy + slab)
- [x] Virtual memory & paging
- [x] SMP + preemptive scheduler
- [x] Scheduler load balancer
- [~] VFS abstraction layer *(in progress)*
- [ ] Ext2 / FAT32 filesystem driver
- [ ] User-space & syscall interface
- [ ] Network stack

---

## License

GPL v2 © Mateja Stefanovic — see [LICENSE](LICENSE) for details.
