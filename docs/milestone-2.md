# ENPY OS milestone 2

We are moving from a minimal boot message to an interrupt-aware kernel foundation. The goals for this milestone are:

- create a valid interrupt descriptor table (IDT)
- initialize the 8259 PIC and prepare the kernel for IRQs
- install a keyboard interrupt entry point
- leave a clean place for the next step: scheduling, memory management, and userland loading

This milestone is intentionally minimal. It is not a complete OS and does not implement real login or disk management yet.

## What is implemented
- GRUB boots the kernel
- text-mode VGA output remains available
- a basic IDT is configured
- PIC initialization is set up
- keyboard interrupt preparation begins
- interrupts are enabled after setup

## What should come next
1. real interrupt service routines for IRQs 0-15
2. keyboard scan code decoding
3. timer tick and scheduler skeleton
4. memory paging and heap setup
5. file-system discovery and device abstractions
6. ENPY shell and app layer

## Why this matters
The kernel must understand interrupts before it can support keyboard input, timers, error handling, and system calls. This is a required step before the ENPY shell and login environment can become real.
