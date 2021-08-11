#include "isr.h"

#include <stdio.h>
#include <stddef.h>

#include "cpu.h"
#include "boot.h"
#include "memory.h"
#include "segment.h"
#include "panic.h"
#include "proc.h"

static void* isr[256] = { NULL };

void isr_init() {
	// Remap the PIC to from 0..15 to 32..47
	out8(0x20, 0x11);
	out8(0xA0, 0x11);
	out8(0x21, 0x20);
	out8(0xA1, 0x28);
	out8(0x21, 0x04);
	out8(0xA1, 0x02);
	out8(0x21, 0x01);
	out8(0xA1, 0x01);
	out8(0x21, 0x00);
	out8(0xA1, 0x00);

	// Mask all interrupts except keyboard
	out8(0x21, 0xFF & ~2);

	// Enable interrupts
	asm volatile ("sti");
}

void isr_set(u8 interrupt, void* handler) {
	isr[interrupt] = handler;
}

void isr_handler(struct isr_stack s) {
	if (s.interrupt >= 32) {
		if (s.interrupt >= 40)
			out8(0xA0, 0x20);
		out8(0x20, 0x20);
		void* handler = isr[s.interrupt];
		if (handler) ((void (*)(struct isr_stack)) handler)(s);
		else printf("Unhandled interrupt 0x%x\n", s.interrupt);
	} else {
		extern struct proc* current_proc;
		if (current_proc->pid == 0)
			panic_isr(s);
		else
			panic_isr_user(s);
	}
}
