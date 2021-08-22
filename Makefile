SYSROOT:=build/rootfs
BINDIR:=$(SYSROOT)/bin
INCDIR:=$(SYSROOT)/include
LIBDIR:=$(SYSROOT)/lib

AS:=x86_64-elf-as
AR:=x86_64-elf-ar
CC:=x86_64-elf-gcc -I$(INCDIR)
LD:=x86_64-elf-ld -L$(LIBDIR)

ASFLAGS:=-g
ARFLAGS:=
CCFLAGS:=-Wall -Wextra -ffreestanding -nostdlib -mno-red-zone -g
LDFLAGS:=-nostdlib

PROG_MOD:=$(shell find programs -mindepth 1 -maxdepth 1 -type d)
MODULES:=boot kernel libc $(PROG_MOD)
PROGRAMS:=$(patsubst programs/%,$(BINDIR)/%,$(PROG_MOD))

all: headers build/hdd.img

headers:
	@mkdir -p "$(INCDIR)"
	@for header in libc/include/*.h; do\
		cp -au $$header $(INCDIR);\
	done

build/hdd.img: build/boot/boot.bin build/kernel/kernel.elf $(PROGRAMS)
	@echo "PACK $@"
	@cp build/boot/boot.bin $@
	@chmod -x $@
	@truncate -s 64M $@
	@tfstool $@ format
	@cp -r kernel/src $(SYSROOT)
	@for entry in `find $(SYSROOT) -mindepth 1`; do\
		if [[ -d "$${entry}" ]]; then\
			tfstool $@ mkdir $${entry:12} || exit 1;\
		else\
			tfstool $@ put $${entry:12} $${entry} || exit 1;\
		fi;\
	done

run: all
	@echo "QEMU build/hdd.img"
	@qemu-system-x86_64 -vga std -drive format=raw,file=build/hdd.img

debug: all
	@echo "QEMU build/hdd.img"
	@qemu-system-x86_64 -drive format=raw,file=build/hdd.img -S -s &
	@echo "GDB build/kernel/kernel.elf"
	@#gdb build/kernel/kernel.elf
	@gdb $(BINDIR)/edit

clean:
	@echo "RM build/"
	@rm -rf build

include $(patsubst %,%/module.mk,$(MODULES))

.PHONY: all run debug clean
