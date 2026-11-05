# ENPY OS

ENPY OS is an original, open-source hobby operating system project. It is not a Windows clone.

> **Status:** Experimental, 32-bit x86 bootable kernel starter. It boots through GRUB and writes directly to VGA text memory. It is not yet a general-purpose OS: there is no real login, multitasking, networking, disk/filesystem support, or hardware driver suite.

## Build and run
See [`docs/bootable-kernel.md`](docs/bootable-kernel.md) for toolchain setup and commands. The first milestone targets an x86 PC emulator (QEMU) and uses GRUB as the bootloader; the kernel itself is a small freestanding C/assembly program.

```sh
make
make run
```

A cross-compiler such as `i686-elf-gcc`, GNU binutils, GRUB utilities, `xorriso`, and QEMU are required. The prototype's browser UI in `ui/` is separate and is not the OS login system.

## Roadmap
1. Bootable kernel and text console (current starter)
2. Interrupts, exceptions, timer, and keyboard input
3. Memory management and a basic heap
4. Process model and system calls
5. Storage and filesystem support
6. Graphics, input, and a native ENPY desktop shell
7. Networking, installer, updates, and broader hardware support

## License
MIT. See [`LICENSE`](LICENSE).
