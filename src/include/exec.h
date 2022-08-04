#ifndef __ELF_H
#define __ELF_H

#include"types.h"
#include"riscv.h"
#include"fat32.h"
#include"elf.h"

int exec(char *path, char **argv, char **env);

#endif
