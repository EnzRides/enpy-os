# Bootable kernel starter (32-bit x86)

This is the first native ENPY OS milestone: a tiny freestanding kernel loaded by GRUB's Multiboot v1 interface. It is intended for learning and emulator testing, not installation on a real machine.

## Dependencies
Install an i686-elf cross toolchain (`i686-elf-gcc`, `i686-elf-as`, and `i686-elf-ld` or the GCC linker driver), GRUB tools (`grub-mkrescue`, `grub-file`), `xorriso`, and QEMU (`qemu-system-i386`). Package names vary by distribution. A cross-compiler is recommended because host compilers may inject host-specific defaults.

## Build an ISO
From the repository root:

```sh
make
```

The build checks the kernel's Multiboot header and creates `build/enpy-os.iso`.

## Run in QEMU

```sh
make run
```

Close the emulator window or press Ctrl-C in the terminal to stop it. You should see an ENPY OS boot message in the VGA text display.

## Clean generated files

```sh
make clean
```

## Important limitations
This is a minimal 32-bit kernel, not a finished desktop operating system. GRUB performs the initial boot loading; this project does not yet include a custom bootloader, real authentication, disk access, filesystems, memory protection, multitasking, networking, or a graphical interface. Do not use it as a daily-use OS or on real hardware. The separate `ui/` browser mockup does not authenticate anyone and is not connected to this kernel.
