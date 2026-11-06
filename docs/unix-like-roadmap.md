# ENPY OS: Unix-like roadmap

ENPY OS is intended to be an independent, Unix-inspired operating system. It is not Linux and does not use the Linux kernel. The practical target is a small freestanding x86 kernel that gradually gains a POSIX-oriented userland.

“Unix” can refer to certified systems and trademarks; this project should be described as **Unix-like** unless it eventually meets the applicable certification requirements. POSIX compatibility also needs to be stated by supported interfaces and tests, not assumed from a shell or desktop appearance.

## Build in dependable milestones

1. **Boot and diagnostics** — reliable Multiboot entry, serial/VGA diagnostics, explicit framebuffer validation, panic screen, and repeatable QEMU smoke tests.
2. **CPU foundations** — GDT/TSS, correct exception handlers, PIC/APIC handling, timer, and keyboard/mouse drivers. Keep interrupts and polling from competing for the same device.
3. **Memory** — parse the bootloader memory map, physical page allocator, kernel heap, then paging and per-process address spaces.
4. **Processes and protection** — ring-3 user mode, context switching, scheduler, process lifecycle, and a documented system-call ABI.
5. **Unix-style kernel interfaces** — file descriptors, `read`/`write`/`close`, process creation and `exec`/`wait`, signals, pipes, permissions, and a VFS. Add interfaces incrementally and test them.
6. **Userland** — a small freestanding C library, init process, shell, basic utilities, and a filesystem image. User programs must run outside the kernel.
7. **Storage and devices** — start with emulated devices and a read-only or disposable disk image; add write support only with tests and safeguards.
8. **Graphics and networking** — keep the desktop as a user-space program once processes, drivers, and IPC exist; develop network support behind an isolated emulator.
9. **Compatibility and release quality** — automated boot tests, kernel tests, POSIX test subsets, documentation, reproducible images, and security review.

## Current scope

The existing repository is an experimental 32-bit x86 kernel/desktop prototype. Its graphical windows are drawn by the kernel and are not yet a full desktop environment. There is no process model, user mode, system-call layer, filesystem, libc, or general hardware support. Each milestone should be built and tested before the next is layered on top.
