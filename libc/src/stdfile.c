#include "stdfile.h"

#include "stddef.h"
#include "stdlib.h"
#include "string.h"
#include "system.h"

static u64 total_blocks = 0;
static u64 bitmap_offset = 0;

static u64 alloc_block() {
	u8 buffer[512];
	if (total_blocks == 0) {
		syscall(SYS_DISK_READ, 0, &buffer);
		total_blocks = ((std_file_t*) &buffer)->total_blocks;
		bitmap_offset = ((std_file_t*) &buffer)->bitmap_offset;
	}
	u64 buffer_block = bitmap_offset;
	for (u64 index = 0; index < total_blocks; index++) {
		if (index % 4096 == 0)
			syscall(SYS_DISK_READ, buffer_block++, &buffer);
		u64 byte = (index % 4096) / 8;
		if (index % 8 == 0 && buffer[byte] == 0xFF) {
			index += 7;
			continue;
		}
		u64 mask = (1 << (7 - (index % 8)));
		if (!(buffer[byte] & mask)) {
			buffer[byte] |= mask;
			syscall(SYS_DISK_WRITE, buffer_block - 1, &buffer);
			return index;
		}
	}
	return 0;
}

static void free_block(u64 index) {
	u8 buffer[512];
	if (bitmap_offset == 0) {
		syscall(SYS_DISK_READ, 0, &buffer);
		bitmap_offset = ((std_file_t*) &buffer)->bitmap_offset;
	}
	u64 block = bitmap_offset + (index / 4096);
	syscall(SYS_DISK_READ, block, &buffer);
	buffer[(index % 4096) / 8] &= ~(1 << (7 - (index % 8)));
	syscall(SYS_DISK_WRITE, block, &buffer);
}

static void clear_file(std_file_t* file) {
	if (file->child == 0)
		return;
	std_file_t iter;
	syscall(SYS_DISK_READ, file->child, &iter);
	while (1) {
		for (int i = 0; i < 63; i++)
			if (iter.pointer[i] != 0)
				free_block(iter.pointer[i]);
		if (iter.pointer[63] == 0)
			break;
		syscall(SYS_DISK_READ, iter.pointer[63], &iter);
	}
	file->size = 0;
}

std_file_t* std_file_open(const char* path, u64 flags) {
	char* rpath = realpath(path);
	if (rpath == NULL)
		return NULL;
	path = rpath + 1;

	std_file_t* block = malloc(sizeof(std_file_t));
	block->index = STD_FILE_ROOT_BLOCK;
	syscall(SYS_DISK_READ, block->index, block);

	char name[STD_FILE_NAME_LENGTH];
	int name_index = 0;
	for (; path[-1]; path++) {
		name[name_index] = *path;
		if (name[name_index] == '/' || name[name_index] == 0) {
			if (name_index == 0)
				continue;
			name[name_index] = 0;
			if (block->type != STD_DIRECTORY)
				goto error;
			if (!std_file_child(block, block, name)) {
				if (*path != 0 || !(flags & STD_FILE_CREATE))
					goto error;
				if (!std_file_add(block, STD_FILE, name))
					goto error;
				if (!std_file_child(block, block, name))
					goto error;
			}
			name_index = 0;
		} else {
			name_index++;
		}
	}

	if (flags & STD_FILE_CLEAR)
		clear_file(block);

	return block;
	error:
		free(block);
		free(rpath);
		return NULL;
}

void std_file_close(std_file_t* file) {
	free(file);
}

bool std_file_parent(std_file_t* file, std_file_t* out) {
	if (file->parent == 0)
		return false;
	if (out != NULL)
		syscall(SYS_DISK_READ, file->parent, out);
	return true;
}

bool std_file_child(std_file_t* file, std_file_t* out, const char* name) {
	if (file->child == 0 || file->type != STD_DIRECTORY)
		return false;
	if (name == NULL) {
		if (out != NULL)
			syscall(SYS_DISK_READ, file->child, out);
		return true;
	}
	std_file_t buffer;
	syscall(SYS_DISK_READ, file->child, &buffer);
	while (1) {
		if (!strcmp(buffer.name, name)) {
			if (out != NULL)
				memcpy(out, &buffer, sizeof(*out));
			return true;
		}
		if (!std_file_next(&buffer, &buffer))
			return false;
	}
}

bool std_file_next(std_file_t* file, std_file_t* out) {
	if (file->next == 0)
		return false;
	if (out != NULL)
		syscall(SYS_DISK_READ, file->next, out);
	return true;
}

bool std_file_add(std_file_t* parent, enum std_file_type type, const char* name) {
	if (parent->type != STD_DIRECTORY)
		return false;
	if (std_file_child(parent, NULL, name))
		return false;

	std_file_t block = { 0 };
	block.index = alloc_block();
	if (block.index == 0)
		return false;
	block.parent = parent->index;
	block.child = 0;
	block.next = 0;
	block.size = 0;
	block.time = 0;
	block.type = type;
	strncpy(block.name, name, STD_FILE_NAME_LENGTH);

	if (parent->child == 0) {
		parent->child = block.index;
		parent->size = 1;
		syscall(SYS_DISK_WRITE, parent->index, parent);
		syscall(SYS_DISK_WRITE, block.index, &block);
		return true;
	}

	std_file_t curr;
	syscall(SYS_DISK_READ, parent->child, &curr);
	if (type >= curr.type && strcmp(name, curr.name) < 0) {
		parent->child = block.index;
		parent->size++;
		syscall(SYS_DISK_WRITE, parent->index, parent);
		block.next = curr.index;
		syscall(SYS_DISK_WRITE, block.index, &block);
		return true;
	}
	while (1) {
		std_file_t prev;
		memcpy(&prev, &curr, sizeof(prev));
		if (!std_file_next(&curr, &curr)) {
			curr.next = block.index;
			syscall(SYS_DISK_WRITE, curr.index, &curr);
			parent->size++;
			syscall(SYS_DISK_WRITE, parent->index, parent);
			syscall(SYS_DISK_WRITE, block.index, &block);
			return true;
		}
		if (type >= curr.type && strcmp(name, curr.name) < 0) {
			prev.next = block.index;
			syscall(SYS_DISK_WRITE, prev.index, &prev);
			parent->size++;
			syscall(SYS_DISK_WRITE, parent->index, parent);
			block.next = curr.index;
			syscall(SYS_DISK_WRITE, block.index, &block);
			return true;
		}
	}
}

bool std_file_remove(std_file_t* block) {
	if (block->type == STD_DIRECTORY && block->size > 0)
		return false;

	clear_file(block);

	std_file_t iter;
	if (!std_file_parent(block, &iter))
		return false;
	iter.size--;
	if (iter.child == block->index) {
		iter.child = block->next;
		syscall(SYS_DISK_WRITE, iter.index, &iter);
	} else {
		syscall(SYS_DISK_WRITE, iter.index, &iter);
		std_file_child(&iter, &iter, NULL);
		while (iter.next != block->index)
			std_file_next(&iter, &iter);
		iter.next = block->next;
		syscall(SYS_DISK_WRITE, iter.index, &iter);
	}
	free_block(block->index);
	return true;
}

u64 std_file_write(std_file_t* file, u64 offset, const void* buffer, u64 length) {
	if (file->type != STD_FILE)
		return 0;

	u64 data_offset = (offset & 0x1FF);
	u64 node_offset = (offset >> 9) % 63;
	u64 node_number = (offset >> 9) / 63;

	std_file_t node = { 0 };
	if (file->child == 0) {
		file->child = alloc_block();
		syscall(SYS_DISK_WRITE, file->index, file);
		syscall(SYS_DISK_WRITE, file->child, &node);
	} else {
		syscall(SYS_DISK_READ, file->child, &node);
	}
	u64 node_index = file->child;
	while (node_number--) {
		if (node.pointer[63] == 0) {
			node.pointer[63] = alloc_block();
			syscall(SYS_DISK_WRITE, node_index, &node);
			node_index = node.pointer[63];
			memset(&node, 0, sizeof(node));
			syscall(SYS_DISK_WRITE, node_index, &node);
		} else {
			node_index = node.pointer[63];
			syscall(SYS_DISK_READ, node_index, &node);
		}
	}

	u8* curr_buf = (u8*) buffer;
	while (length) {
		if (node.pointer[node_offset] == 0)
			node.pointer[node_offset] = alloc_block();
		u64 to_write = length < (512 - data_offset) ? length : (512 - data_offset);

		if (to_write == 512) {
			syscall(SYS_DISK_WRITE, node.pointer[node_offset], curr_buf);
		} else {
			u8 data[512];
			syscall(SYS_DISK_READ, node.pointer[node_offset], &data);
			memcpy(&data[data_offset], curr_buf, to_write);
			syscall(SYS_DISK_WRITE, node.pointer[node_offset], &data);
			data_offset = 0;
		}
		curr_buf += to_write;
		length -= to_write;

		if (++node_offset == 63) {
			if (node.pointer[node_offset] == 0) {
				node.pointer[node_offset] = alloc_block();
				syscall(SYS_DISK_WRITE, node_index, &node);
				node_index = node.pointer[node_offset];
				memset(&node, 0, sizeof(node));
				syscall(SYS_DISK_WRITE, node_index, &node);
			} else {
				node_index = node.pointer[node_offset];
				syscall(SYS_DISK_READ, node_index, &node);
			}
			node_offset = 0;
		}
	}
	syscall(SYS_DISK_WRITE, node_index, &node);

	u64 written = (u64) curr_buf - (u64) buffer;
	if (offset + written > file->size)
		file->size = offset + written;
	file->time = 0;
	syscall(SYS_DISK_WRITE, file->index, file);
	return written;
}

u64 std_file_read(std_file_t* file, u64 offset, void* buffer, u64 length) {
	if (file->type != STD_FILE)
		return 0;
	if (file->child == 0)
		return 0;
	if (length == 0)
		return 0;

	u64 data_offset = (offset & 0x1FF);
	u64 node_offset = (offset >> 9) % 63;
	u64 node_number = (offset >> 9) / 63;

	std_file_t node = { 0 };
	syscall(SYS_DISK_READ, file->child, &node);
	u64 node_index = file->child;
	while (node_number--) {
		if (node.pointer[63] == 0)
			return 0;
		node_index = node.pointer[63];
		syscall(SYS_DISK_READ, node_index, &node);
	}

	u8* curr_buf = (u8*) buffer;
	while (length) {
		if (node.pointer[node_offset] == 0)
			return curr_buf - (u8*) buffer;
		u64 to_read = length < (512 - data_offset) ? length : (512 - data_offset);

		if (to_read == 512) {
			syscall(SYS_DISK_READ, node.pointer[node_offset], curr_buf);
		} else {
			u8 data[512];
			syscall(SYS_DISK_READ, node.pointer[node_offset], &data);
			memcpy(curr_buf, &data[data_offset], to_read);
			data_offset = 0;
		}
		curr_buf += to_read;
		length -= to_read;

		if (++node_offset == 63) {
			if (node.pointer[63] == 0)
				return curr_buf - (u8*) buffer;
			node_index = node.pointer[63];
			syscall(SYS_DISK_READ, node_index, &node);
			node_offset = 0;
		}
	}

	return curr_buf - (u8*) buffer;
}
