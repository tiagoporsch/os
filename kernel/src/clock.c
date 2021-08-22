#include "clock.h"

#include <stdint.h>

#include "cpu.h"
#include "isr.h"

void clock_isr() {}

void clock_init() {
	isr_set(0x20, clock_isr);
	u16 divisor = 1193;
	out8(0x43, 0x36);
	out8(0x40, (u8) (divisor & 0xFF));
	out8(0x40, (u8) ((divisor >> 8) & 0xFF));
}
