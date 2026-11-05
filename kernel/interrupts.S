.section .text

.global isr_stub
.type isr_stub, @function
isr_stub:
    cli
    hlt
    iret

.global idt_flush
.type idt_flush, @function
idt_flush:
    mov 4(%esp), %eax
    lidt (%eax)
    ret
