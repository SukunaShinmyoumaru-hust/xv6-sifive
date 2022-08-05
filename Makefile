K=src
U=usrinit
SBI=sbi/fw_jump.elf
LINKER=linker/kernel.ld

FS?=FAT
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
	$K/copy.o \
	$(DISK) \
	$K/ramdisk.o \
	$K/spi.o \
	$K/sd.o \
	$K/diskio.o \
	$K/disk.o \
	$K/string.o \
	$K/intr.o \
	$K/cpu.o \
	$K/image.o \
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
	$K/timer.o \
	$K/main.o \
	$K/kernelvec.o \
	$K/trap.o \
	$K/proc.o \
	$K/vma.o\
	$K/uarg.o \
	$K/exec.o \
	$K/sysfile.o \
	$K/sysproc.o \
	$K/syssig.o \
	$K/syscall.o

TOOLPREFIX=riscv64-linux-gnu-

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

$K/kernel:sys $(OBJS) $(LINKER)
	@$(LD) $(LDFLAGS) -T $(LINKER) -o $K/kernel $(OBJS)
	@$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	@$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym


$K/bin.S:$U/_sacrifice $U/initcode

sys:
	./syscall/sys.sh

$U/initcode:$U/initcode.S
	$(CC) $(CFLAGS) -march=rv64g -nostdinc -I. -Isrc -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode
	rm -f $U/initcode.o $U/initcode.out $U/initcode.d

clean:
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*/*.o */*.d */*.asm */*.sym src/include/sysnum.h src/syscall.c \
	$U/_* $U/initcode $U/usys.S $K/kernel	

ULIB = $U/usys.o $U/printf.o

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

$U/usys.S : sys

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

disk:
	@rm -f *.sb

dst=/mnt

disk.img:disk
	@rm -f disk.img
	@if [ ! -f "disk.img" ]; then \
		echo "making disk image..."; \
		dd if=/dev/zero of=disk.img bs=512k count=8; \
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

commit?=wait

add:
	git remote add origin https://gitlab.eduxiji.net/Cty/oskernrl2022-rv6.git

push:
	git add .
	git commit -m  "$(commit)"
	git push origin master

