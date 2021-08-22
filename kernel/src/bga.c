#include "bga.h"

#include <stddef.h>
#include <stdio.h>

#include "cpu.h"
#include "memory.h"
#include "pci.h"
#include "panic.h"

#define VBE_DISPI_IOPORT_INDEX	0x1CE
#define VBE_DISPI_IOPORT_DATA	0x1CF

#define VBE_DISPI_INDEX_ID		0x0
#define VBE_DISPI_INDEX_XRES	0x1
#define VBE_DISPI_INDEX_YRES	0x2
#define VBE_DISPI_INDEX_BPP		0x3
#define VBE_DISPI_INDEX_ENABLE	0x4

#define VBE_DISPI_ID0			0xB0C0
#define VBE_DISPI_ID4			0xB0C4

#define VBE_DISPI_DISABLED		0x00
#define VBE_DISPI_ENABLED		0x01
#define VBE_DISPI_LFB_ENABLED	0x40

static u32* framebuffer;

static void bga_write(u16 index, u16 data) {
	out16(VBE_DISPI_IOPORT_INDEX, index);
	out16(VBE_DISPI_IOPORT_DATA, data);
}

static u16 bga_read(u16 index) {
	out16(VBE_DISPI_IOPORT_INDEX, index);
	return in16(VBE_DISPI_IOPORT_DATA);
}

void bga_init() {
	u16 version = bga_read(VBE_DISPI_INDEX_ID);
	if (version != VBE_DISPI_ID0)
		panic("invalid BGA version %04x", version);
	u32 device = pci_scan(0x0300);
	if (device == (u32) -1)
		panic("no VGA adapter");
	u32 addr = pci_read(device, PCI_BAR0, 4) & 0xFFFFFFF0;
	u64 pages = BGA_WIDTH * BGA_HEIGHT * BGA_BPP / (4 * 4096);
	framebuffer = memory_map(memory_pm_get(), addr, NULL, pages);

	bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
	bga_write(VBE_DISPI_INDEX_XRES, BGA_WIDTH);
	bga_write(VBE_DISPI_INDEX_YRES, BGA_HEIGHT);
	bga_write(VBE_DISPI_INDEX_BPP, BGA_BPP);
	bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
}

void bga_clear(u32 c) {
	for (u32 i = 0; i < BGA_WIDTH * BGA_HEIGHT; i++)
		framebuffer[i] = c;
}

void bga_draw_pixel(int x, int y, u32 c) {
	framebuffer[x + y * BGA_WIDTH] = c;
}

void bga_draw_rect(int x, int y, int w, int h, u32 c) {
	for (int i = 0; i < w; i++) {
		bga_draw_pixel(x + i, y, c);
		bga_draw_pixel(x + i, y + h - 1, c);
	}
	for (int i = 0; i < h; i++) {
		bga_draw_pixel(x, y + i, c);
		bga_draw_pixel(x + w - 1, y + i, c);
	}
}
