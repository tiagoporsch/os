#include "segment.h"

#include "memory.h"

static struct idt_entry idt[256] = { 0 };
static union gdt_entry gdt[5] = { 0 };
static struct tss tss = { 0 };

void segment_init() {
	// Interrupt Descriptor Table
	extern int isr_vector;
	u64 base = (u64) &isr_vector;
	for (int i = 0; i < 48; i++) {
		idt[i].base_low = base & 0xFFFF;
		idt[i].base_mid = (base >> 16) & 0xFFFF;
		idt[i].base_high = (base >> 32) & 0xFFFFFFFF;
		idt[i].selector = 0x8;
		idt[i].ist = 0x1;
		idt[i].flags = 0x8E;
		base += 16;
	}

	// Task State Segment
	tss.ist[0] = MEM_AT_PHYS(0x5000);

	// Global Descriptor Table
	gdt[0].raw = 0;
	gdt[1].raw = 0x00209A0000000000;
	gdt[2].raw = 0x0000920000000000;
	gdt[3].limit_low = sizeof(tss);
	gdt[3].limit_high = (sizeof(tss) >> 8) & 0xF;
	gdt[3].base_low = (u64) &tss & 0xFFFF;
	gdt[3].base_mid = ((u64) &tss >> 16) & 0xFF;
	gdt[3].base_high = ((u64) &tss >> 24) & 0xFF;
	gdt[4].base_higher = ((u64) &tss >> 32) & 0xFFFFFFFF;
	gdt[3].access = 0x89;
	gdt[3].flags = 0x4;

	// Load descriptors
	struct seg_desc idtr;
	idtr.limit = (u16) sizeof(idt);
	idtr.base = (u64) &idt;
	asm volatile ("lidt %0" : : "m" (idtr));

	struct seg_desc gdtr;
	gdtr.limit = (u16) sizeof(gdt);
	gdtr.base = (u64) &gdt;
	asm volatile ("lgdt %0" : : "m" (gdtr));

	asm volatile ("movw $0x18, %ax; ltr %ax");
}
