#include <stddef.h>
#include <stdfile.h>
#include <stdlib.h>
#include <stdio.h>
#include <system.h>

int main() {
	std_file_t super;
	syscall(SYS_DISK_READ, 0, &super);

	u64 total_size = super.total_blocks * 512;
	u8* bitmap = (u8*) &super;
	u64 bitmap_blocks = super.bitmap_blocks;
	u64 bitmap_offset = super.bitmap_offset;
	u64 used_size = 0;
	for (u64 block = 0; block < bitmap_blocks; block++) {
		syscall(SYS_DISK_READ, bitmap_offset + block, bitmap);
		for (int i = 0; i < 512; i++) {
			if (bitmap[i] == 0xFF) {
				used_size += 512 * 8;
			} else for (int j = 0; j < 8; j++) {
				if (bitmap[i] & (1 << j)) {
					used_size += 512;
				}
			}
		}
	}

	u64 percent_used = (100 * used_size) / total_size;

	char total_unit = 'B';
	if (total_size >= 1024) { total_size /= 1024; total_unit = 'K'; }
	if (total_size >= 1024) { total_size /= 1024; total_unit = 'M'; }
	if (total_size >= 1024) { total_size /= 1024; total_unit = 'G'; }

	char used_unit = 'B';
	if (used_size >= 1024) { used_size /= 1024; used_unit = 'K'; }
	if (used_size >= 1024) { used_size /= 1024; used_unit = 'M'; }
	if (used_size >= 1024) { used_size /= 1024; used_unit = 'G'; }

	printf("Disk usage: %d%c of %d%c (%d%%)\n",
		used_size, used_unit, total_size, total_unit, percent_used
	);
	return 0;
}
