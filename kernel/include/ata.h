#pragma once

#include <stdint.h>

struct ata_identify {
	u16 type;
	u8 unused1[52];
	i8 model[40];
	u8 unused2[4];
	u16 capabilities;
	u8 unused3[20];
	u32 max_lba;
	u8 unused4[76];
	u64 max_lba_ext;
	u8 unused5[304];
} __attribute__ ((packed));

struct ata_device {
	u32 base;
	u32 control;
	u8 slave;
	u8 is_atapi;
	struct ata_identify identify;
};

void ata_init();
struct ata_device* ata_get_device(int);
void ata_read_sector(struct ata_device*, u64, void*);
void ata_write_sector(struct ata_device*, u64, const void*);
