#include "ata.h"

#include <stddef.h>
#include <stdio.h>

#include "cpu.h"
#include "panic.h"

#define ATA_SR_BSY		0x80
#define ATA_SR_DRDY		0x40
#define ATA_SR_DF		0x20
#define ATA_SR_DSC		0x10
#define ATA_SR_DRQ		0x08
#define ATA_SR_CORR		0x04
#define ATA_SR_IDX		0x02
#define ATA_SR_ERR		0x01

#define ATA_CMD_READ_PIO		0x20
#define ATA_CMD_READ_PIO_EXT	0x24
#define ATA_CMD_READ_DMA		0xC8
#define ATA_CMD_READ_DMA_EXT	0x25
#define ATA_CMD_WRITE_PIO		0x30
#define ATA_CMD_WRITE_PIO_EXT	0x34
#define ATA_CMD_WRITE_DMA		0xCA
#define ATA_CMD_WRITE_DMA_EXT	0x35
#define ATA_CMD_CACHE_FLUSH		0xE7
#define ATA_CMD_CACHE_FLUSH_EXT	0xEA
#define ATA_CMD_PACKET			0xA0
#define ATA_CMD_IDENTIFY_PACKET	0xA1
#define ATA_CMD_IDENTIFY		0xEC

#define ATA_REG_DATA		0x00
#define ATA_REG_ERROR		0x01
#define ATA_REG_FEATURES	0x01
#define ATA_REG_SECCOUNT0	0x02
#define ATA_REG_LBA0		0x03
#define ATA_REG_LBA1		0x04
#define ATA_REG_LBA2		0x05
#define ATA_REG_HDDEVSEL	0x06
#define ATA_REG_COMMAND		0x07
#define ATA_REG_STATUS		0x07
#define ATA_REG_SECCOUNT1	0x08
#define ATA_REG_LBA3		0x09
#define ATA_REG_LBA4		0x0A
#define ATA_REG_LBA5		0x0B
#define ATA_REG_CONTROL		0x0C
#define ATA_REG_ALTSTATUS	0x0C
#define ATA_REG_DEVADDRESS	0x0D

struct ata_device ata_devices[4] = {
	{ .base = 0x1F0, .control = 0x3F6, .slave = 0 },
	{ .base = 0x1F0, .control = 0x3F6, .slave = 1 },
	{ .base = 0x170, .control = 0x376, .slave = 0 },
	{ .base = 0x170, .control = 0x376, .slave = 1 },
};

struct ata_device* ata_get_device(int index) {
	return index < 4 ? &ata_devices[index] : NULL;
}

static void ata_io_wait(struct ata_device *device) {
	for (int i = 0; i < 4; i++)
		in8(device->base + ATA_REG_ALTSTATUS);
}

static void ata_device_detect(struct ata_device* device) {
	out8(device->control, 1 << 1);
	ata_io_wait(device);

	out8(device->base + ATA_REG_HDDEVSEL, 0xA0 | device->slave << 4);
	ata_io_wait(device);

	out8(device->base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	ata_io_wait(device);

	if (!in8(device->base + ATA_REG_STATUS)) return;
	while (in8(device->base + ATA_REG_STATUS) & ATA_SR_BSY);

	u16 c = (in8(device->base + ATA_REG_LBA2) << 8) | in8(device->base + ATA_REG_LBA1);
	if (c == 0xFFFF) return;
	if (c == 0xEB14 || c == 0x9669) {
		device->is_atapi = 1;
		out8(device->base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
		ata_io_wait(device);
	}

	out8(device->base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	ata_io_wait(device);

	ins32(device->base + ATA_REG_DATA, (u64) &device->identify, 128);
	for (int i = 0; i < 40; i += 2) {
		char c = device->identify.model[i];
		device->identify.model[i] = device->identify.model[i + 1];
		device->identify.model[i + 1] = c;
	}
	char* k = &device->identify.model[39];
	while (*k == ' ')
		*k-- = '\0';
}

void ata_init() {
	for (int i = 0; i < 4; i++)
		ata_device_detect(&ata_devices[i]);
}

static void ata_pio_out_lba(struct ata_device* device, u64 lba, u16 count) {
	out8(device->control, (1 << 7) | (1 << 1));
	out8(device->base + ATA_REG_SECCOUNT1 - 6, 0);
	out8(device->base + ATA_REG_LBA3 - 6, (lba >> 24) & 0xFF);
	out8(device->base + ATA_REG_LBA4 - 6, (lba >> 32) & 0xFF);
	out8(device->base + ATA_REG_LBA5 - 6, (lba >> 40) & 0xFF);
	out8(device->control, (1 << 1));
	out8(device->base + ATA_REG_SECCOUNT0, count & 0xFF);
	out8(device->base + ATA_REG_LBA0, (lba >>  0) & 0xFF);
	out8(device->base + ATA_REG_LBA1, (lba >>  8) & 0xFF);
	out8(device->base + ATA_REG_LBA2, (lba >> 16) & 0xFF);
}

void ata_read_sector(struct ata_device* device, u64 lba, void* buffer) {
	if (!(device->identify.capabilities & 0x200))
		panic("Device does not support LBA\n");
	while (in8(device->base + ATA_REG_STATUS) & ATA_SR_BSY);
	out8(device->base + ATA_REG_HDDEVSEL, 0xE0 | (device->slave << 4));
	ata_pio_out_lba(device, lba, 1);
	out8(device->base + ATA_REG_COMMAND, ATA_CMD_READ_PIO_EXT);
	ata_io_wait(device);
	while (in8(device->base + ATA_REG_STATUS) & ATA_SR_BSY);
	ins32(device->base + ATA_REG_DATA, (u64) buffer, 128);
}

void ata_write_sector(struct ata_device* device, u64 lba, const void* buffer) {
	if (!(device->identify.capabilities & 0x200))
		panic("Device does not support LBA\n");
	while (in8(device->base + ATA_REG_STATUS) & ATA_SR_BSY);
	out8(device->base + ATA_REG_HDDEVSEL, 0xE0 | (device->slave << 4));
	ata_pio_out_lba(device, lba, 1);
	out8(device->base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO_EXT);
	ata_io_wait(device);
	while (in8(device->base + ATA_REG_STATUS) & ATA_SR_BSY);
	outs32(device->base + ATA_REG_DATA, (u64) buffer, 128);
	out8(device->base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH_EXT);
}
