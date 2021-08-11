BOOT_SRC:=$(shell find boot/src -type f \( -name "*.c" -or -name "*.s" \))
BOOT_OBJ:=$(patsubst boot/src/%,build/boot/%.o,$(BOOT_SRC))
BOOT_LNK:=boot/src/linker.ld

build/boot/boot.bin: $(BOOT_LNK) $(BOOT_OBJ)
	@mkdir -p "$(@D)"
	@echo "LD $@"
	@$(LD) --oformat binary -o $@ -T $(BOOT_LNK) $(BOOT_OBJ)

build/boot/%.s.o: boot/src/%.s
	@mkdir -p "$(@D)"
	@echo "AS $@"
	@$(AS) $(ASFLAGS) -o $@ $<

build/boot/%.c.o: boot/src/%.c
	@mkdir -p "$(@D)"
	@echo "CC $@"
	@$(CC) $(CCFLAGS) -Iboot/include -MD -c -o $@ $<

-include $(BOOT_OBJ:.o=.d)
