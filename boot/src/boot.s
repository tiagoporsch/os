.code16
.section .boot
.global boot
boot:
	# Disable interrupts
	cli

	# Set up the stack (..0x6FFF)
	xor %ax, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov $0x7000, %sp

	# Clear the screen and reset the cursor
	mov $0, %ah
	mov $3, %al
	int $0x10

	# Check if extended read is available
	mov $0x41, %ah
	mov $0x55AA, %bx
	mov $0x80, %dl
	int $0x13
	jc err_biosext

	# Read sectors from the disk
	mov $0x42, %ah
	mov $dap, %si
	movw $_boot_sectors, 2(%si)
	mov $0x80, %dl
	int $0x13
	jc err_disk

	# Jump to the next stage
	jmp boot16

print_string:
	mov $0xE, %ah
0:	lodsb
	or %al, %al
	jz 0f
	int $0x10
	jmp 0b
0:	ret

0: .string "error: bios extensions"
err_biosext:
	mov $0b, %si
	call print_string
0:	hlt
	jmp 0b

0: .string "error: reading disk"
err_disk:
	mov $0b, %si
	call print_string
0:	hlt
	jmp 0b

dap:
	.word 0x10	# Size of the data address packet
	.word 0x0	# How many sectors to read
	.word 0x0	# Destination offset
	.word 0x7E0 # Destination segment
	.quad 0x1	# LBA to be read

.fill 0x1E6 - (. - boot), 1, 0
.quad 0 # Total blocks
.quad 0 # Bitmap blocks
.quad 0 # Bitmap offset
.word 0xAA55 # Boot signature

.section .text
boot16:
	# Get total memory after 0x100000
	xor %ebx, %ebx
0:	movl $0xE820, %eax
	movl $0x500, %edi
	movl $0x18, %ecx
	movl $0x534D4150, %edx
	int $0x15
	jc err_mmap
	test %ebx, %ebx
	je err_nomem
	mov %es:(%di), %eax
	or %es:4(%di), %eax
	cmp $0x100000, %eax
	jne 0b
	mov %es:8(%di), %eax
	or %es:12(%di), %eax
	mov %eax, (0x7000)
	mov %es:16(%di), %eax
	or %es:20(%di), %eax
	mov %eax, (0x7008)

	# Identity map the first gigabyte using one huge page
	mov $0x1000, %edi
	mov $0x800, %ecx
	xor %eax, %eax
	cld
	rep stosl
	movl $0x2003, (0x1000)
	movl $0x0083, (0x2000)

	# Disable the programmable interrupt controller
	mov $0xFF, %al
	out %al, $0xA1
	out %al, $0x21

	# Set PAE, PGE, OSFXSR and OSXMMEXCPT bits
	mov $0b11010100000, %eax
	mov %eax, %cr4

	# Load the page map
	mov $0x1000, %edx
	mov %edx, %cr3

	# Set the LME bit
	mov $0xC0000080, %ecx
	rdmsr
	or $0x100, %eax
	wrmsr

	# Clear EM and set MP bits
	mov %cr0, %ebx
	and $~(1 << 3), %ebx
	or $0x80000011, %ebx
	mov %ebx, %cr0

	# Load the GDT and TSS
	lgdt gdt

	# Long mode jump
	jmp $8, $boot64

0: .string "error: query system address map"
err_mmap:
	mov $0b, %si
	call print_string
0:	hlt
	jmp 0b

0: .string "error: no memory after 0x10000"
err_nomem:
	mov $0b, %si
	call print_string
0:	hlt
	jmp 0b

# Global Descriptor Table
.align 8
gdt_base:
	.quad 0x0000000000000000
	.quad 0x00209A0000000000
	.quad 0x0000920000000000
gdt:
	.word . - gdt_base
	.quad gdt_base

# Long mode code
.code64
boot64:
	# Set up segment registers
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss

	# Add 1MB to total memory
	addq $0x100000, (0x7000)

	# Map memory
	call memory_init

	# Load kernel
	movq $kernel_path, %rdi
	call elf_load

	# Set stack
	movq $0xFFFFFFC000000000, %rsp

	# Jump to kernel!
	jmp *%rax

kernel_path: .string "/boot/kernel.elf"
