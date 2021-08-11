.global syscall_handler
syscall_handler:
	pushq %rcx
	movq %r10, %rcx
	movq $8, %rbx
	mul %rbx
	movabsq $syscall_handlers, %rbx
	addq %rbx, %rax
	call *(%rax)
	popq %rcx
	jmp *%rcx
