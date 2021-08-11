.global _start
_start:
	popq %rax
	movq %rax, (stdin)
	popq %rax
	movq %rax, (stdout)

	call _libc_init_heap

	popq %rdi
	popq %rsi
	call main

	movq %rax, %rdi
	call exit
