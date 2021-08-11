#pragma once

#include <stdbool.h>
#include <stdint.h>

enum proc_status {
	PROC_RUNNING, PROC_READY, PROC_DONE,
};

struct proc {
	u64 rax, rbx, rcx, rdx;
	u64 rsi, rdi, rsp, rbp;
	u64 cr3;

	u64 pid, waitpid;
	u64 ret;
	void* stack;
	char cwd[256];

	enum proc_status status;
	struct proc* prev;
	struct proc* next;
};

void proc_init();

u64 proc_exec(const char*, const char**);
void proc_yield();
void proc_exit(u64);
u64 proc_wait(u64);

char* proc_getcwd(char*);
bool proc_chdir(const char*);
