#pragma once

#include <stdint.h>

#define PCI_VENDOR_ID		0x00
#define PCI_HEADER_TYPE		0x0E
#define PCI_SUBCLASS		0x0A
#define PCI_CLASS			0x0B
#define PCI_BAR0			0x10
#define PCI_SECONDARY_BUS	0x19

u32 pci_read(u32, u8, u8);
u32 pci_scan(u16);
