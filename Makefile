K=src
U=usrinit
SBI=sbi/fw_jump.elf
LINKER=linker/kernel.ld

FS?=RAM
MAC?=SIFIVE_U

ifeq ($(MAC),SIFIVE_U)
DISK:=$K/link_null.o
endif

ifeq ($(MAC),QEMU)
DISK:=$K/link_disk.o
endif

OBJS += \
	$K/entry.o \
	$K/bio.o \
	$(DISK) \
	$K/ramdisk.o \
	$K/spi.o \
	$K/sd.o \
	$K/diskio.o \
	$K/disk.o \
	$K/string.o \
	$K/intr.o \
	$K/image.o \
	$K/proc.o \
	$K/fat32.o \
	$K/pipe.o \
	$K/file.o \
	$K/bin.o \
	$K/dev.o \
	$K/swtch.o \
	$K/trampoline.o \
	$K/sig_trampoline.o \
	$K/signal.o \
	$K/spinlock.o \
	$K/sleeplock.o \
	$K/printf.o \
	$K/pm.o \
	$K/kmalloc.o \
	$K/vm.o \
	$K/plic.o \
	$K/timer.o \
	$K/main.o \
	$K/kernelvec.o \
	$K/trap.o \
	$K/copy.o \
	$K/poll.o \
	$K/cpu.o \
	$K/vma.o \
	$K/mmap.o \
	$K/uarg.o \
	$K/exec.o \
	$K/uname.o\
	$K/sysfile.o \
	$K/systime.o \
	$K/sysproc.o \
	$K/syslog.o \
	$K/syspoll.o \
	$K/syssig.o \
	$K/sysmem.o \
	$K/syscall.o

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif


QEMU = qemu-system-riscv64

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -DDEBUG -DWARNING -DERROR -D$(FS) -D$(MAC)
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I. -I./src
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

LDFLAGS = -z max-page-size=4096
	
# $K/link_app.o:fs.img

$K/kernel:$K/syscall.c $(OBJS) $(LINKER)
	@$(LD) $(LDFLAGS) -T $(LINKER) -o $K/kernel $(OBJS)
	@$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	@$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

binmake:$U/initcode
	./bin/bin.sh

$K/bin.S:binmake

$K/syscall.c:
	./syscall/sys.sh

$U/initcode:$U/initcode.S
	$(CC) $(CFLAGS) -march=rv64g -nostdinc -I. -Isrc -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode
	rm -f $U/initcode.o $U/initcode.out $U/initcode.d

dump%:
	@$(OBJDUMP) -S sd/$* > dump/$*.s
	@readelf -a sd/$* > dump/$*.txt

rebench:
	rm -f sd/lmbench_all sd/lmbench_all.txt  sd/lmbench_all.asm
	make -C lmbench clean
	make -C lmbench all
	cp ./lmbench/bin/XXX/lmbench_all ./sd/lmbench_all
	make dumplmbench_all
	make disk.img

rebench-gdb:
	rm -f sd/lmbench_all sd/lmbench_all.txt  sd/lmbench_all.asm
	make -C lmbench clean
	make -C lmbench debug-all
	cp ./lmbench/bin/XXX/lmbench_all ./sd/lmbench_all
	make dumplmbench_all
	make disk.img

clean:
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*/*.o */*.d */*.asm */*.sym src/include/sysnum.h src/syscall.c src/bin.S \
	$U/_* $U/initcode $U/usys.S $K/kernel	

ULIB = $U/usys.o $U/printf.o $U/lua_test.o $U/lmbench_test.o $U/busybox_test.o

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

$U/usys.S : $K/syscall.c

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

all:$K/kernel
	riscv64-unknown-elf-objcopy -O binary -S $K/kernel os.bin

disk:
	@rm -f *.sb

dst=/mnt

disk.img:disk
	@rm -f disk.img
	@if [ ! -f "disk.img" ]; then \
		echo "making disk image..."; \
		dd if=/dev/zero of=disk.img bs=512k count=16; \
		mkfs.vfat -F 32 disk.img; fi
	@sudo mount disk.img $(dst)
	@sudo cp -r sd/* $(dst)
	@sudo umount $(dst)

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)	

ifndef CPUS
CPUS := 5
endif


ifndef M
M = sifive_u
endif


QEMUOPTS = -machine $(M) -bios $(SBI) -kernel $K/kernel -smp $(CPUS) -nographic

qemu: $K/kernel
	$(QEMU) $(QEMUOPTS)

.gdbinit: .gdbinit.tmpl-riscv
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

qemu-gdb: $K/kernel .gdbinit
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

commit?=gpipeh

add:
	git remote add origin https://gitlab.eduxiji.net/Cty/oskernrl2022-rv6.git

push:
	git add .
	git commit -m  "$(commit)"
	git push origin gpipe





