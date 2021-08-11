#pragma once

#include <stdint.h>

struct isr_stack {
	u64 cr0, cr2, cr3, cr4;
	u64 rdi, rsi, rbp;
	u64 rax, rbx, rcx, rdx;
	u64 interrupt, error_code;
	u64 rip, cs, flags, rsp, ss;
};

void isr_init();
void isr_set(u8, void*);
