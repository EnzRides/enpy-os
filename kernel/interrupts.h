#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

void init_idt(void);
void init_pic(void);
void init_keyboard(void);
void enable_interrupts(void);
void disable_interrupts(void);

#endif
