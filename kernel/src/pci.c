#include "pci.h"

#include <stdio.h>

#include "cpu.h"
#include "panic.h"

#define PCI_CONFIG_ADDRESS	0xCF8
#define PCI_CONFIG_DATA		0xCFC

#define PCI_BUS(d) ((u8) (d >> 16))
#define PCI_SLOT(d) ((u8) (d >> 8))
#define PCI_FUNCTION(d) ((u8) (d))
#define PCI_DEVICE(b, s, f) ((u32) ((b << 16) | (s << 8) | f))

u32 pci_read(u32 device, u8 field, u8 size) {
	u32 addr = 0x80000000;
	addr |= PCI_BUS(device) << 16;
	addr |= PCI_SLOT(device) << 11;
	addr |= PCI_FUNCTION(device) << 8;
	addr |= field & 0xFC;
	out32(PCI_CONFIG_ADDRESS, addr);
	if (size == 1) return in8(PCI_CONFIG_DATA + (field & 3));
	if (size == 2) return in16(PCI_CONFIG_DATA + (field & 2));
	if (size == 4) return in32(PCI_CONFIG_DATA);
	panic("invalid size for pci_read: %d\n", size);
}

u32 pci_scan_bus(u16, u8);
u32 pci_scan_function(u16 type, u8 bus, u8 slot, u8 function) {
	u32 device = PCI_DEVICE(bus, slot, function);
	u16 device_type = pci_read(device, PCI_CLASS, 1) << 8 | pci_read(device, PCI_SUBCLASS, 1);
	if (device_type == type)
		return device;
	if (device_type == 0x0604)
		return pci_scan_bus(type, pci_read(device, PCI_SECONDARY_BUS, 1));
	return -1;
}

u32 pci_scan_slot(u16 type, u8 bus, u8 slot) {
	u32 device = PCI_DEVICE(bus, slot, 0);
	if (pci_read(device, PCI_VENDOR_ID, 2) == 0xFFFF)
		return -1;
	if (pci_scan_function(type, bus, slot, 0) != (u32) -1)
		return device;
	if (!pci_read(device, PCI_HEADER_TYPE, 1))
		return -1;
	for (u8 function = 1; function < 8; function++) {
		u32 device = PCI_DEVICE(bus, slot, function);
		if (pci_read(device, PCI_VENDOR_ID, 2) != 0xFFFF)
			if (pci_scan_function(type, bus, slot, function) != (u32) -1)
				return device;
	}
	return -1;
}

u32 pci_scan_bus(u16 type, u8 bus) {
	for (u8 slot = 0; slot < 32; slot++) {
		u32 out = pci_scan_slot(type, bus, slot);
		if (out != (u32) -1)
			return out;
	}
	return -1;
}

u32 pci_scan(u16 type) {
	if (!(pci_read(0, PCI_HEADER_TYPE, 1) & 0x80))
		return pci_scan_bus(type, 0);
	for (u8 function = 0; function < 8; function++) {
		u32 device = PCI_DEVICE(0, 0, function);
		if (pci_read(device, PCI_VENDOR_ID, 2) == 0xFFFF)
			break;
		u32 out = pci_scan_bus(type, function);
		if (out != (u32) -1)
			return out;
	}
	return -1;
}
