.macro ISR_NOERR n
.align 16
	cli
	push $0
	push \n
	jmp isr_common
.endm

.macro ISR_ERR n
.align 16
	cli
	push \n
	jmp isr_common
.endm

.align 16
.global isr_vector
isr_vector:
	ISR_NOERR $0
	ISR_NOERR $1
	ISR_NOERR $2
	ISR_NOERR $3
	ISR_NOERR $4
	ISR_NOERR $5
	ISR_NOERR $6
	ISR_NOERR $7
	ISR_ERR $8
	ISR_NOERR $9
	ISR_ERR $10
	ISR_ERR $11
	ISR_ERR $12
	ISR_ERR $13
	ISR_ERR $14
	ISR_NOERR $15
	ISR_NOERR $16
	ISR_ERR $17
	ISR_NOERR $18
	ISR_NOERR $19
	ISR_NOERR $20
	ISR_NOERR $21
	ISR_NOERR $22
	ISR_NOERR $23
	ISR_NOERR $24
	ISR_NOERR $25
	ISR_NOERR $26
	ISR_NOERR $27
	ISR_NOERR $28
	ISR_NOERR $29
	ISR_NOERR $30
	ISR_NOERR $31
	ISR_NOERR $32
	ISR_NOERR $33
	ISR_NOERR $34
	ISR_NOERR $35
	ISR_NOERR $36
	ISR_NOERR $37
	ISR_NOERR $38
	ISR_NOERR $39
	ISR_NOERR $40
	ISR_NOERR $41
	ISR_NOERR $42
	ISR_NOERR $43
	ISR_NOERR $44
	ISR_NOERR $45
	ISR_NOERR $46
	ISR_NOERR $47

isr_common:
	push %rdx
	push %rcx
	push %rbx
	push %rax

	push %rbp
	push %rsi
	push %rdi

	mov %cr4, %rax
	push %rax
	mov %cr3, %rax
	push %rax
	mov %cr2, %rax
	push %rax
	mov %cr0, %rax
	push %rax

	call isr_handler

	pop %rax # cr0
	pop %rax # cr2
	pop %rax # cr3
	pop %rax # cr4

	pop %rdi
	pop %rsi
	pop %rbp

	pop %rax
	pop %rbx
	pop %rcx
	pop %rdx

	addq $16, %rsp
	sti
	iretq
