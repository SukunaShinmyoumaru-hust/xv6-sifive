#ifndef __CONSOLE_H
#define __CONSOLE_H
#include "types.h"

void consoleinit(void);
void consputc(int c);
void consoleintr(int c);
void checkchar(void);
void endcheck(void);

#endif
