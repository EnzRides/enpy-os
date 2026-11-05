/* Multiboot v1 entry with a requested 800x600x32 graphics mode. */
.set MBALIGN,    1 << 0
.set MEMINFO,    1 << 1
.set VIDEO,      1 << 2
.set MBFLAGS,    MBALIGN | MEMINFO | VIDEO
.set MBMAGIC,    0x1BADB002
.set MBCHECKSUM, -(MBMAGIC + MBFLAGS)

.section .multiboot,"a"
.align 4
.long MBMAGIC
.long MBFLAGS
.long MBCHECKSUM
.long 0                 /* linear graphics mode */
.long 800
.long 600
.long 32

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
    cli
    mov $stack_top, %esp
    push %ebx            /* Multiboot info pointer, cdecl argument */
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
.size _start, . - _start

.section .note.GNU-stack,"",@progbits
