#include "stdlib.h"

#include "assert.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"
#include "system.h"

#define HEAP_MAGIC 0xDEADBEEFFEEBDAED
#define HEAP_PAGES 16

static struct heap_node {
	u64 magic;
	u64 size : 63;
	u64 free :  1;
	struct heap_node* prev;
	struct heap_node* next;
} __attribute__ ((packed)) *heap_head;

void _libc_init_heap() {
	heap_head = mmap(NULL, HEAP_PAGES);
	heap_head->magic = HEAP_MAGIC;
	heap_head->size = 0x1000 * HEAP_PAGES - sizeof(struct heap_node);
	heap_head->free = true;
	heap_head->prev = NULL;
	heap_head->next = NULL;
}

static bool can_concat(struct heap_node* node) {
	return ((u64) node->next) == ((u64) node) + node->size + sizeof(*node);
}

void* malloc(u64 size) {
	while (1) {
		struct heap_node* node = heap_head;
		while (1) {
			assert(node->magic == HEAP_MAGIC);
			if (node->free) {
				u64 addr = (u64) node + sizeof(struct heap_node);
				if (node->size == size) {
					node->free = false;
					return (void*) addr;
				} else if (node->size > size + sizeof(struct heap_node)) {
					struct heap_node* next = (struct heap_node*) (addr + size);
					next->magic = HEAP_MAGIC;
					next->size = node->size - sizeof(struct heap_node) - size;
					next->free = true;
					next->prev = node;
					next->next = node->next;
					if (next->next != NULL)
						next->next->prev = next;

					node->size = size;
					node->free = false;
					node->next = next;
					return (void*) addr;
				}
			}
			if (node->next == NULL)
				break;
			node = node->next;
		}

		u64 pages = HEAP_PAGES;
		while (pages * 0x1000 < size)
			pages += HEAP_PAGES;
		node->next = mmap(NULL, pages);
		printf("new heap at %x (from %x)\n", node->next, heap_head);
		node->next->magic = HEAP_MAGIC;
		node->next->size = 0x1000 * pages - sizeof(struct heap_node);
		node->next->free = true;
		node->next->prev = node;
		node->next->next = NULL;
	}
}

void* calloc(u64 size) {
	void* data = malloc(size);
	if (data != NULL)
		memset(data, 0, size);
	return data;
}

void* realloc(void* addr, u64 size) {
	if (addr == NULL)
		return malloc(size);
	struct heap_node* node = (void*) (addr - sizeof(struct heap_node));
	assert(node->magic == HEAP_MAGIC);
	i64 size_diff = size - node->size;
	if (size_diff <= 0)
		return addr;
	if (node->next != NULL && node->next->free && can_concat(node)) {
		struct heap_node* next = node->next;
		assert(next->magic == HEAP_MAGIC);
		if (next->size == size_diff - sizeof(struct heap_node)) {
			next->magic = 0;
			node->size += sizeof(struct heap_node) + next->size;
			if (next->next != NULL) {
				assert(next->next->magic == HEAP_MAGIC);
				next->next->prev = node;
			}
			node->next = next->next;
			return addr;
		} else if (next->size > size_diff) {
			next->magic = 0;
			next = (struct heap_node*) (addr + size);
			memmove(next, node->next, sizeof(struct heap_node));
			next->magic = HEAP_MAGIC;
			next->size -= size_diff;
			node->size += size_diff;
			node->next = next;
			return addr;
		}
	}
	void* new_addr = malloc(size);
	if (new_addr == NULL)
		return NULL;
	memcpy(new_addr, addr, node->size);
	free(addr);
	return new_addr;
}

void free(void* addr) {
	struct heap_node* node = (void*) (addr - sizeof(struct heap_node));
	assert(node->magic == HEAP_MAGIC);
	node->free = true;
	if (node->next != NULL && node->next->free && can_concat(node)) {
		node->next->magic = 0;
		node->size += sizeof(struct heap_node) + node->next->size;
		if (node->next->next != NULL) {
			assert(node->next->next->magic == HEAP_MAGIC);
			node->next->next->prev = node;
		}
		node->next = node->next->next;
	}
	if (node->prev != NULL && node->prev->free && can_concat(node->prev)) {
		node->magic = 0;
		node->prev->size += sizeof(struct heap_node) + node->size;
		if (node->next != NULL) {
			assert(node->next->magic == HEAP_MAGIC);
			node->next->prev = node->prev;
		}
		node->prev->next = node->next;
	}
}

char* realpath(const char* path) {
	if (path == NULL)
		return NULL;
	u64 len = strlen(path);
	if (path[len - 1] == '/')
		len--;
	char* str = (char*) path;
	if (path[0] != '/') {
		char* cwd = getcwd(NULL);
		u64 cwd_len = strlen(cwd);
		if (cwd_len == 1) cwd_len = 0;
		str = malloc(cwd_len + len + 2);
		memcpy(str, cwd, cwd_len);
		str[cwd_len] = '/';
		memcpy(str + cwd_len + 1, path, len);
		len = cwd_len + len + 1;
		str[len] = 0;
		free(cwd);
	}

	char buf[256] = {0};
	u64 buf_len = 0;

	buf[buf_len++] = '/';
	for (u64 i = 1; i <= len; i++) {
		if (i == len || str[i] == '/') {
			char* base = strrchr(buf, '/') + 1;
			u64 base_len = buf_len - (u64) (base - buf);
			if (base_len == 0) {
				if (str != path)
					free(str);
				return NULL;
			} else if (!strncmp(base, ".", base_len)) {
				buf[buf_len -= (base_len + 1)] = 0;
			} else if (!strncmp(base, "..", base_len)) {
				buf[buf_len -= (base_len + 1)] = 0;
				while (buf_len && buf[--buf_len] != '/');
				buf[buf_len] = 0;
			}
			if (i < len || buf_len == 0) {
				buf[buf_len++] = '/';
				buf[buf_len] = 0;
			}
		} else {
			buf[buf_len++] = str[i];
		}
	}
	buf[buf_len] = 0;

	if (str != path)
		free(str);
	return strdup(buf);
}

void yield() {
	syscall(SYS_YIELD);
}

_Noreturn void exit(u64 ret) {
	while (1) syscall(SYS_EXIT, ret);
}
