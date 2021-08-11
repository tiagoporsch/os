#pragma once

#include <stdint.h>

union gdt_entry {
	struct {
		u16 limit_low;
		u16 base_low;
		u8 base_mid;
		u8 access;
		u8 limit_high : 4;
		u8 flags : 4;
		u8 base_high;
	};
	struct {
		u32 base_higher;
		u32 reserved;
	};
	u64 raw;
} __attribute__ ((packed));

struct idt_entry {
	u16 base_low;
	u16 selector;
	u8 ist;
	u8 flags;
	u16 base_mid;
	u32 base_high;
	u32 zero;
} __attribute__ ((packed));

struct seg_desc {
	u16 limit;
	u64 base;
} __attribute__ ((packed));

struct tss {
	u32 _reserved0;
	u64 rsp[3];
	u64 _reserved1;
	u64 ist[7];
	u64 _reserved2;
	u32 _reserved3;
} __attribute__ ((packed));

void segment_init();
