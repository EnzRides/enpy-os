#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

void init_idt(void);
void init_pic(void);
void init_keyboard(void);
int keyboard_poll(void);
void enable_interrupts(void);
void disable_interrupts(void);
void keyboard_irq_handler(void);
void keyboard_scancode_handler(uint8_t code);

#endif
