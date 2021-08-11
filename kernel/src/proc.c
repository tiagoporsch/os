#include "proc.h"

#include <stddef.h>
#include <stdfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"
#include "memory.h"
#include "panic.h"

struct proc* kernel_proc;
struct proc* current_proc;

void proc_init() {
	current_proc = malloc(sizeof(*current_proc));
	current_proc->cr3 = memory_pm_get();
	strcpy(current_proc->cwd, "/");
	current_proc->pid = 0;
	current_proc->waitpid = 0;
	current_proc->status = PROC_RUNNING;
	current_proc->prev = current_proc;
	current_proc->next = current_proc;
	kernel_proc = current_proc;
}

u64 proc_exec(const char* path, const char** argv) {
	struct proc* proc = malloc(sizeof(*proc));
	proc->cr3 = memory_pm_new();

	void* entry = elf_load(proc->cr3, path);
	if (entry == NULL) {
		memory_pm_free(proc->cr3);
		free(proc);
		return 0;
	}

	proc->stack = memory_alloc(proc->cr3, NULL, 2);
	u64* stack = memory_share(memory_pm_get(), proc->cr3, proc->stack, 2);

	u64 argc; for (argc = 0; argv && argv[argc]; argc++);
	u64 offset = argc * sizeof(char*);
	for (u64 i = 0; i < argc; i++) {
		stack[i] = (u64) proc->stack + offset;
		u64 len = strlen(argv[i]) + 1;
		strncpy(((char*) stack) + offset, argv[i], len);
		offset += len;
	}

	u64 sp = 1024;
	stack[--sp] = (u64) proc->stack;
	stack[--sp] = (u64) argc;
	stack[--sp] = (u64) stdout;
	stack[--sp] = (u64) stdin;
	stack[--sp] = (u64) entry;
	stack[--sp] = (u64) 0;
	proc->rsp = (u64) proc->stack + sizeof(u64) * sp;
	memory_unmap(memory_pm_get(), stack, 2);

	static u64 pid = 1;
	strncpy(proc->cwd, current_proc->cwd, sizeof(proc->cwd));
	proc->pid = pid++;
	proc->waitpid = 0;
	proc->status = PROC_READY;
	proc->prev = current_proc;
	proc->next = current_proc->next;
	current_proc->next->prev = proc;
	current_proc->next = proc;

	return proc->pid;
}

void proc_switch(struct proc*, struct proc*);
void proc_yield() {
	struct proc* next = current_proc->next;
	while (1) {
		if (next->waitpid == 0 && next->status == PROC_READY)
			break;
		if (next == current_proc)
			return;
		next = next->next;
	}

	struct proc* curr = current_proc;
	if (curr->status == PROC_RUNNING)
		curr->status = PROC_READY;
	next->status = PROC_RUNNING;

	current_proc = next;
	proc_switch(curr, next);
}

void proc_exit(u64 ret) {
	if (current_proc->pid == 0)
		panic("attempted to exit kernel");
	current_proc->ret = ret;
	current_proc->status = PROC_DONE;
	struct proc* proc = current_proc->next;
	while (proc != current_proc) {
		if (proc->waitpid == current_proc->pid)
			proc->waitpid = 0;
		proc = proc->next;
	}
	proc_yield();
}

u64 proc_wait(u64 pid) {
	if (pid == 0)
		return -1;
	struct proc* proc = current_proc;
	while (proc->pid != pid) {
		proc = proc->next;
		if (proc == current_proc)
			return -1;
	}
	current_proc->waitpid = pid;
	proc_yield();
	current_proc->waitpid = 0;
	proc->prev->next = proc->next;
	proc->next->prev = proc->prev;
	memory_pm_free(proc->cr3);
	u64 ret = proc->ret;
	free(proc);
	return ret;
}

char* proc_getcwd(char* cwd) {
	if (cwd == NULL)
		return strdup(current_proc->cwd);
	strcpy(cwd, current_proc->cwd);
	return cwd;
}

bool proc_chdir(const char* path) {
	char* real = realpath(path);
	std_file_t* file = std_file_open(real, 0);
	if (file != NULL) {
		if (file->type == STD_DIRECTORY) {
			strncpy(current_proc->cwd, real, sizeof(current_proc->cwd));
			std_file_close(file);
			free(real);
			return true;
		}
		std_file_close(file);
	}
	free(real);
	return false;
}
