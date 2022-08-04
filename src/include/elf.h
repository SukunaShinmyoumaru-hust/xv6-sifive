#ifndef __ELF_H
#define __ELF_H

#include"types.h"
#include"riscv.h"
#include"fat32.h"
// Format of an ELF executable file
#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
  uint magic;  // must equal ELF_MAGIC
  uchar elf[12];
  ushort type;
  ushort machine;
  uint version;
  uint64 entry;
  uint64 phoff;
  uint64 shoff;
  uint flags;
  ushort ehsize;
  ushort phentsize;
  ushort phnum;
  ushort shentsize;
  ushort shnum;
  ushort shstrndx;
};

// Program section header
struct proghdr {
  uint32 type;
  uint32 flags;
  uint64 off;
  uint64 vaddr;
  uint64 paddr;
  uint64 filesz;
  uint64 memsz;
  uint64 align;
};

struct sechdr{
  uint32 name;
  uint32 type;
  uint64 flags;
  uint64 addr;
  uint64 offset;
  uint64 size;
  uint32 link;
  uint32 info;
  uint64 addralign;
  uint64 entsize;
};

struct relaent{
  uint64 offset;
  uint64 info;
  uint64 addend;
};

struct syment{
  uint16 name; 
  char info;
  char other;
  uint32 shndx;
  uint64 value;
  uint64 size;
};

struct symentry{
  uint32 name;
  uint16 info;
  uint16 ndx;
  uint64 value;
  uint32 size;
};

#define SELFLOAD

#define INFO2SYMIDX(x) (x>>32)

#define SYMTYPE(x) (x&0xf)
#define SYMBIND(x) (x&0xf0)
#define SYMVIS(x)  (x&0xf00)

#define SYM_NOTYPE		0
#define SYM_OBJECT		1
#define SYM_FUNC		2
#define SYM_SECTION		3
#define SYM_FILE		4

#define BIND_LOCAL		0
#define BIND_GLOBAL		1
#define BIND_WEAK		2

#define NDX_ABS		0xfff1

#define VIS_DEFAULT		0
#define VIS_HIDDEN		2

// Values for Proghdr type
#define ELF_PROG_PHDR           6
#define ELF_PROG_LOAD           1
#define ELF_PROG_INTERP         3
#define ELF_PROG_DYNAMIC        2
#define ELF_PROG_TLS            7
#define ELF_GNU_STACK           1685382481
#define ELF_GNU_RELRO           1685382482

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4

#define AUX_CNT	 32

#define AT_NULL		0
#define AT_IGNORE	1
#define AT_EXECFD	2
#define AT_PHDR		3
#define AT_PHENT	4
#define AT_PHNUM	5
#define AT_PAGESZ	6
#define AT_BASE		7
#define AT_FLAGS	8
#define AT_ENTRY	9
#define AT_NOTELF	10
#define AT_UID		11
#define AT_EUID		12
#define AT_GID		13
#define AT_EGID		14
#define AT_CLKTCK	17


#define AT_PLATFORM	15
#define AT_HWCAP	16




#define AT_FPUCW	18


#define AT_DCACHEBSIZE	19
#define AT_ICACHEBSIZE	20
#define AT_UCACHEBSIZE	21



#define AT_IGNOREPPC	22

#define	AT_SECURE	23

#define AT_BASE_PLATFORM 24

#define AT_RANDOM	25

#define AT_HWCAP2	26

#define AT_EXECFN	31



#define AT_SYSINFO	32
#define AT_SYSINFO_EHDR	33

#endif
