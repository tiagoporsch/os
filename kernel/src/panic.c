#include "panic.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "tty.h"

_Noreturn void panic(const char *format, ...) {
	printf("\033[91mKERNEL PANIC\n");
	printf("\033[97m* ");
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n* \033[0mSystem halted.\n");
	tty_flush();
	while (1) asm ("cli; hlt");
}

static const char* exception_name[] = {
	"Integer Divide-by-Zero Exception",				// 0
	"Debug Exception",								// 1
	"Non-Maskable-Interrupt",						// 2
	"Breakpoint Exception (INT 3)",					// 3
	"Overflow Exception (INTO instruction)",		// 4
	"Bound-Range Exception (BOUND instruction)",	// 5
	"Invalid-Opcode Exception",						// 6
	"Device-Not-Available Exception",				// 7
	"Double-Fault Exception",						// 8
	"Coprocessor-Segment-Overrun Exception",		// 9
	"Invalid-TSS Exception",						// 10
	"Segment-Not-Present Exception",				// 11
	"Stack Exception",								// 12
	"General-Protection Exception",					// 13
	"Page-Fault Exception",							// 14
	"(Reserved)",									// 15
	"x87 Floating-Point Exception",					// 16
	"Alignment-Check Exception",					// 17
	"Machine-Check Exception",						// 18
	"SIMD Floating-Point Exception",				// 19
};
static const u64 exception_count = sizeof(exception_name) / sizeof(*exception_name);

_Noreturn void panic_isr(struct isr_stack s) {
	printf("\033[91mKERNEL PANIC\n");

	if (s.interrupt < exception_count) {
		printf("\033[97m* %s\033[0m\n", exception_name[s.interrupt]);
	} else {
		printf("\033[97m* unknown exception \033[0m(int:\033[97m%d\033[0m)\n");
	}
	switch (s.interrupt) {
		case 14: // page-fault exception
			printf("  caused by a \033[97m");
			printf(s.error_code & 1 ? "": "non-present ");
			printf(s.error_code & 2 ? "write" : "read");
			printf("\033[0m, while on \033[97m%s\033[0m mode", s.error_code & 4 ? "user" : "kernel");
			printf(", at \033[97m0x%x\033[0m", s.cr2);
			printf(s.error_code & 8 ? ", on a reserved page\n" : "\n");
			break;
		default:
			if (s.error_code != 0)
				printf("  error code: %d\n", s.error_code);
			break;
	}

	printf("\033[97m* \033[0mFull Context:\n");
	printf("    RAX:%16x RDI:%16x CR0:%16x\n", s.rax, s.rdi, s.cr0);
	printf("    RBX:%16x RSI:%16x CR2:%16x\n", s.rbx, s.rsi, s.cr2);
	printf("    RCX:%16x RSP:%16x CR3:%16x\n", s.rcx, s.rsp, s.cr3);
	printf("    RDX:%16x RBP:%16x CR4:%16x\n", s.rdx, s.rbp, s.cr4);
	printf("    RIP:%16x  CS:%6x SS:%6x FLG:%16x\n", s.rip, s.cs, s.ss, s.flags);
	printf("\033[97m* \033[0mSystem halted.\n");

	tty_flush();
	while (1) asm ("cli; hlt");
}

_Noreturn void panic_isr_user(struct isr_stack s) {
	if (s.interrupt < exception_count) {
		printf("\033[91m* %s\033[0m\n", exception_name[s.interrupt]);
	} else {
		printf("\033[91m* Unknown Exception \033[0m(INT:\033[97m%d\033[0m)\n");
	}
	switch (s.interrupt) {
		case 14: // Page-Fault Exception
			printf("  Caused by a \033[97m");
			printf(s.error_code & 1 ? "": "non-present ");
			printf(s.error_code & 2 ? "write" : "read");
			printf("\033[0m, while on \033[97m%s\033[0m mode", s.error_code & 4 ? "user" : "kernel");
			printf(", at \033[97m0x%x\033[0m", s.cr2);
			printf(s.error_code & 8 ? ", on a reserved page\n" : "\n");
			break;
		default:
			if (s.error_code != 0)
				printf("  Error Code: %d\n", s.error_code);
			break;
	}
	printf("\033[97m* \033[0mFull Context:\n");
	printf("    RAX:%16x RDI:%16x CR0:%16x\n", s.rax, s.rdi, s.cr0);
	printf("    RBX:%16x RSI:%16x CR2:%16x\n", s.rbx, s.rsi, s.cr2);
	printf("    RCX:%16x RSP:%16x CR3:%16x\n", s.rcx, s.rsp, s.cr3);
	printf("    RDX:%16x RBP:%16x CR4:%16x\n", s.rdx, s.rbp, s.cr4);
	printf("    RIP:%16x  CS:%6x SS:%6x FLG:%16x\n", s.rip, s.cs, s.ss, s.flags);
	exit(139);
}
