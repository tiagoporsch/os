#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <system.h>

#include "ata.h"
#include "bga.h"
#include "clock.h"
#include "keyboard.h"
#include "memory.h"
#include "isr.h"
#include "proc.h"
#include "segment.h"
#include "syscall.h"
#include "tty.h"

void _libc_init_heap();

void kernel_main() {
	syscall_init();
	memory_init();

	_libc_init_heap();
	stdin = stream_open(NULL);
	stdout = stream_open(NULL);

	segment_init();
	tty_init();
	isr_init();
	clock_init();
	keyboard_init();
	ata_init();
	proc_init();
	bga_init();

	bga_clear(0xAA0000);
	bga_draw_rect(100, 100, 80, 24, 0xFFFFFF);

	proc_exec("/bin/init", NULL);
	while (1) {
		if (!tty_flush()) {
			asm ("hlt");
			yield();
		}
	}
}
