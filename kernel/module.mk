KERNEL_SRC:=$(shell find kernel/src -type f \( -name "*.c" -or -name "*.s" \))
KERNEL_OBJ:=$(patsubst kernel/src/%,build/kernel/%.o,$(KERNEL_SRC))
KERNEL_LNK:=kernel/src/linker.ld

build/kernel/kernel.elf: build/libc/libc.a $(KERNEL_LNK) $(KERNEL_OBJ)
	@mkdir -p "$(@D)"
	@echo "LD $@"
	@$(LD) -o $@ -T $(KERNEL_LNK) $(KERNEL_OBJ) -lc
	@mkdir -p "$(SYSROOT)/boot"
	@cp $@ "$(SYSROOT)/boot"

build/kernel/%.s.o: kernel/src/%.s
	@mkdir -p "$(@D)"
	@echo "AS $@"
	@$(AS) $(ASFLAGS) -o $@ $<

build/kernel/%.c.o: kernel/src/%.c
	@mkdir -p "$(@D)"
	@echo "CC $@"
	@$(CC) $(CCFLAGS) -Ikernel/include -mcmodel=large -MD -c -o $@ $<

-include $(KERNEL_OBJ:.o=.d)
