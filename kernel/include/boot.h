#pragma once

#include "memory.h"
#include "stdint.h"

#define BOOT_INFO ((struct boot_info*) MEM_AT_PHYS(0x7000))

struct boot_info {
	u64 total_memory;
};
