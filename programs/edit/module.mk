EDIT_SRC:=$(shell find programs/edit/src -type f -name "*.c")
EDIT_OBJ:=$(patsubst programs/edit/src/%,build/programs/edit/%.o,$(EDIT_SRC))

$(BINDIR)/edit: build/libc/libc.a $(EDIT_OBJ)
	@mkdir -p "$(@D)"
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -o $@ $(filter-out build/libc/libc.a,$^) -lc

build/programs/edit/%.s.o: programs/edit/src/%.s
	@mkdir -p "$(@D)"
	@echo "AS $@"
	@$(AS) $(ASFLAGS) -o $@ $<

build/programs/edit/%.c.o: programs/edit/src/%.c
	@mkdir -p "$(@D)"
	@echo "CC $@"
	@$(CC) $(CCFLAGS) -Iprograms/edit/include -MD -c -o $@ $<

-include $(EDIT_OBJ:.o=.d)
