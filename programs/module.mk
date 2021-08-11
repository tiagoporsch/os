$(PROG)_SRC:=$(shell find programs/$(PROG)/src -type f -name "*.c")
$(PROG)_OBJ:=$(patsubst programs/$(PROG)/src/%,build/programs/$(PROG)/%.o,$($(PROG)_SRC))

$(BINDIR)/$(PROG): build/libc/libc.a $($(PROG)_OBJ)
	@mkdir -p "$(@D)"
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -o $@ $(filter-out build/libc/libc.a,$^) -lc

build/programs/$(PROG)/%.s.o: programs/$(PROG)/src/%.s
	@mkdir -p "$(@D)"
	@echo "AS $@"
	@$(AS) $(ASFLAGS) -o $@ $<

build/programs/$(PROG)/%.c.o: programs/$(PROG)/src/%.c
	@mkdir -p "$(@D)"
	@echo "CC $@"
	@$(CC) $(CCFLAGS) -Iprograms/$(PROG) -MD -c -o $@ $<

-include $($(PROG)_OBJ:.o=.d)
