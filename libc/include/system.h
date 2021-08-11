#pragma once

#include "stdbool.h"
#include "stdint.h"

enum syscall {
	SYS_YIELD, SYS_EXIT,
	SYS_EXEC, SYS_WAIT,
	SYS_GETCWD, SYS_CHDIR,

	SYS_MMAP, SYS_MUNMAP,

	SYS_DISK_READ, SYS_DISK_WRITE,
};

u64 syscall(enum syscall, ...);

static inline u64 exec(char* path, char** argv) {
	return syscall(SYS_EXEC, path, argv);
}

static inline u64 wait(u64 pid) {
	return syscall(SYS_WAIT, pid);
}

static inline char* getcwd(char* buffer) {
	return (char*) syscall(SYS_GETCWD, buffer);
}

static inline bool chdir(const char* path) {
	return (bool) syscall(SYS_CHDIR, path);
}

static inline void* mmap(void* vaddr, u64 size) {
	return (void*) syscall(SYS_MMAP, vaddr, size);
}

static inline void munmap(void* vaddr, u64 size) {
	syscall(SYS_MUNMAP, vaddr, size);
}
