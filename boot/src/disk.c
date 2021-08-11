#include "disk.h"

#include <stddef.h>

#include "string.h"

static inline void outb(u16 port, u8 data) {
	asm volatile ("outb %0, %1" : : "a" (data), "Nd" (port));
}

static inline u8 inb(u16 port) {
	u8 ret;
	asm volatile ("inb %1, %0" : "=a" (ret) : "Nd" (port));
	return ret;
}

static void disk_read(u64 lba, void* buffer) {
	while (inb(0x1F7) & 0x80);
	outb(0x1F6, 0xE0);
	outb(0x3F6, (1 << 7) | (1 << 1));
	outb(0x1F8 - 6, 0);
	outb(0x1F9 - 6, (lba >> 24) & 0xFF);
	outb(0x1FA - 6, (lba >> 32) & 0xFF);
	outb(0x1FB - 6, (lba >> 40) & 0xFF);
	outb(0x3F6, (1 << 1));
	outb(0x1F2, 1);
	outb(0x1F3, (lba >>  0) & 0xFF);
	outb(0x1F4, (lba >>  8) & 0xFF);
	outb(0x1F5, (lba >> 16) & 0xFF);
	outb(0x1F7, 0x24);
	for (int i = 0; i < 4; i++) inb(0x1FC);
	while (inb(0x1F7) & 0x80);
	asm volatile ("cld; rep; insl" : : "D" (buffer), "d" (0x1F0), "c" (128));
}

bool file_find(union file* block, const char* path) {
	if (*path++ != '/')
		return false;

	disk_read(FILE_ROOT_BLOCK, block);

	char name[FILE_NAME_LENGTH];
	int name_index = 0;
	for (; path[-1]; path++) {
		name[name_index] = *path;
		if (name[name_index] == '/' || name[name_index] == 0) {
			if (name_index == 0)
				continue;
			name[name_index] = 0;
			if (block->type != FILE_DIRECTORY)
				return false;
			if (!file_child(block, block, name))
				return false;
			name_index = 0;
		} else {
			name_index++;
		}
	}

	return true;
}

bool file_next(union file* file, union file* out) {
	if (file->next == 0)
		return false;
	if (out != NULL)
		disk_read(file->next, out);
	return true;
}

bool file_child(union file* file, union file* out, const char* name) {
	if (file->child == 0 || file->type != FILE_DIRECTORY)
		return false;
	if (name == NULL) {
		if (out != NULL)
			disk_read(file->child, out);
		return true;
	}
	union file buffer;
	disk_read(file->child, &buffer);
	while (1) {
		if (!strcmp(buffer.name, name)) {
			if (out != NULL)
				memcpy(out, &buffer, sizeof(*out));
			return true;
		}
		if (!file_next(&buffer, &buffer))
			return false;
	}
	return true;
}

u64 file_read(union file* file, u64 offset, void* buffer, u64 length) {
	if (file->child == 0)
		return 0;
	if (length == 0)
		return 0;

	u64 data_offset = (offset & 0x1FF);
	u64 node_offset = (offset >> 9) % 63;
	u64 node_number = (offset >> 9) / 63;

	union file node = { 0 };
	disk_read(file->child, &node);
	u64 node_index = file->child;
	while (node_number--) {
		if (node.pointer[63] == 0)
			return 0;
		node_index = node.pointer[63];
		disk_read(node_index, &node);
	}

	u8* curr_buf = (u8*) buffer;
	while (length) {
		if (node.pointer[node_offset] == 0)
			return curr_buf - (u8*) buffer;
		u64 to_read = length < (512 - data_offset) ? length : (512 - data_offset);

		if (to_read == 512) {
			disk_read(node.pointer[node_offset], curr_buf);
		} else {
			u8 data[512];
			disk_read(node.pointer[node_offset], &data);
			memcpy(curr_buf, &data[data_offset], to_read);
			data_offset = 0;
		}
		curr_buf += to_read;
		length -= to_read;

		if (++node_offset == 63) {
			if (node.pointer[63] == 0)
				return curr_buf - (u8*) buffer;
			node_index = node.pointer[63];
			disk_read(node_index, &node);
			node_offset = 0;
		}
	}

	return curr_buf - (u8*) buffer;
}
