LIBC_SRC:=$(shell find libc/src -type f \( -name "*.c" -or -name "*.s" \) )
LIBC_OBJ:=$(patsubst libc/src/%,build/libc/%.o,$(LIBC_SRC))

build/libc/libc.a: $(LIBC_OBJ)
	@mkdir -p "$(@D)"
	@echo "AR $@"
	@$(AR) rcs $@ $^
	@mkdir -p "$(LIBDIR)"
	@cp $@ "$(LIBDIR)"

build/libc/%.s.o: libc/src/%.s
	@mkdir -p "$(@D)"
	@echo "AS $@"
	@$(AS) $(ASFLAGS) -o $@ $<

build/libc/%.c.o: libc/src/%.c
	@mkdir -p "$(@D)"
	@echo "CC $@"
	@$(CC) $(CCFLAGS) -mcmodel=large -MD -c -o $@ $<

-include $(LIBC_OBJ:.o=.d)
