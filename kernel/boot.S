/* Multiboot v1 entry point. GRUB enters in 32-bit protected mode. */
.set MBALIGN,    1 << 0
.set MEMINFO,    1 << 1
.set MBFLAGS,    MBALIGN | MEMINFO
.set MBMAGIC,    0x1BADB002
.set MBCHECKSUM, -(MBMAGIC + MBFLAGS)

.section .multiboot
.align 4
.long MBMAGIC
.long MBFLAGS
.long MBCHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

.section .text
.global _start
.type _start, @function
.extern kernel_main
_start:
    mov $stack_top, %esp
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
.size _start, . - _start
