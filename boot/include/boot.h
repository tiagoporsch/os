#pragma once

#include "stdint.h"

#define BOOT_INFO ((struct boot_info*) 0x7000)

struct boot_info {
	u64 total_memory;
};
