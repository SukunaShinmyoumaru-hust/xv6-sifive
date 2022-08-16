//
// formatted console output -- printf, panic.
//

#include <stdarg.h>

#include "include/types.h"
#include "include/param.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/console.h"
#include "include/sbi.h"
#include "include/printf.h"

volatile int panicked = 0;

static char digits[] = "0123456789abcdef";
static char warningstr[] = "[WARNING]";
static char errorstr[] = "[ERROR]";

// lock to avoid interleaving concurrent printf's.
static struct {
  struct spinlock lock;
  int locking;
} pr;

void consputc(int c) {
  if(c == BACKSPACE){
    // if the user typed backspace, overwrite with a space.
    sbi_console_putchar('\b');
    sbi_console_putchar(' ');
    sbi_console_putchar('\b');
  } else {
    sbi_console_putchar(c);
  }
}

void printstring(const char* s) {
    while (*s)
    {
        consputc(*s++);
    }
}

static void
printint(int xx, int base, int sign)
{
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}


static void
printptr(uint64 x)
{
  int i;
  consputc('0');
  consputc('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the console. only understands %d, %x, %p, %s.
void
printf(char *fmt, ...)
{
  va_list ap;
  int i, c;
  int locking;
  char *s;
  locking = pr.locking;
  if(locking)
    acquire(&pr.lock);
  
  if (fmt == 0)
    panic("null fmt");

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'c':
      consputc(va_arg(ap, int));
      break;
    case 'd':
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      printptr(va_arg(ap, uint64));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
    
  }
  if(locking)
    release(&pr.lock);
    
}

void
panic(char *s)
{
  printf("panic: ");
  printf(s);
  printf("\n");
  backtrace();
  panicked = 1; // freeze uart output from other CPUs
  for(;;)
    ;
}

void backtrace()
{
  uint64 *fp = (uint64 *)r_fp();
  uint64 *bottom = (uint64 *)PGROUNDUP((uint64)fp);
  printf("backtrace:\n");
  while (fp < bottom) {
    uint64 ra = *(fp - 1);
    printf("%p\n", ra - 4);
    fp = (uint64 *)*(fp - 2);
  }
  printf("!TEST FINISH!\n");
}

void
__debug_info(char *fmt, ...){
#ifdef DEBUG
  va_list ap;
  int i, c;
  int locking;
  char *s;
  locking = pr.locking;
  if(locking)
    acquire(&pr.lock);
  
  if (fmt == 0)
    panic("null fmt");

  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'c':
      consputc(va_arg(ap, int));
      break;
    case 'd':
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      printptr(va_arg(ap, uint64));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
    
  }
  if(locking)
    release(&pr.lock);
#endif    
}

void
__debug_warn(char *fmt, ...){
#ifdef WARNING
  va_list ap;
  int i, c;
  int locking;
  char *s;
  locking = pr.locking;
  if(locking)
    acquire(&pr.lock);
  
  if (fmt == 0)
    panic("null fmt");
  printstring(warningstr);
  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'c':
      consputc(va_arg(ap, int));
      break;
    case 'd':
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      printptr(va_arg(ap, uint64));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
    
  }
  if(locking)
    release(&pr.lock);
#endif
}

void
__debug_error(char *fmt, ...){
#ifdef ERROR
  va_list ap;
  int i, c;
  int locking;
  char *s;
  locking = pr.locking;
  if(locking)
    acquire(&pr.lock);
  
  if (fmt == 0)
    panic("null fmt");

  printstring(errorstr);
  va_start(ap, fmt);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'c':
      consputc(va_arg(ap, int));
      break;
    case 'd':
      printint(va_arg(ap, int), 10, 1);
      break;
    case 'x':
      printint(va_arg(ap, int), 16, 1);
      break;
    case 'p':
      printptr(va_arg(ap, uint64));
      break;
    case 's':
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
    
  }
  if(locking)
    release(&pr.lock);
  
  backtrace();
  panicked = 1; // freeze uart output from other CPUs
  for(;;)
    ;
    
#endif
}

void
printfinit(void)
{
  initlock(&pr.lock, "pr");
  pr.locking = 1;   // changed, used to be 1
}

#ifdef SIFIVE_U
void print_logo() {
/*
    printf(" (`-')           (`-')                 (`-').->  _                 _           (`-') (`-')  _                    \n");
    printf(" (OO )_.->      _(OO )                 ( OO)_   (_)       <-.     (_)         _(OO ) ( OO).-/              .->   \n");
    printf(" (_| \\_)--.,--.(_/,-.\\  ,--.  (`-')   (_)--\\_)  ,-(`-')(`-')-----.,-(`-'),--.(_/,-.\\(,------.         ,--.(,--.  \n");
    printf(" \\  `.'  / \\   \\ / (_/ /  .'  ( OO).->/    _ /  | ( OO)(OO|(_\\---'| ( OO)\\   \\ / (_/ |  .---'         |  | |(`-')\n");
    printf("  \\    .')  \\   /   / .  / -.(,------.\\_..`--.  |  |  ) / |  '--. |  |  ) \\   /   / (|  '--.    (`-') |  | |(OO )\n");
    printf("  .'    \\  _ \\     /_)'  .-. \\`------'.-._)   \\(|  |_/  \\_)  .--'(|  |_/ _ \\     /_) |  .--' <-.(OO ) |  | | |  \\\n");
    printf(" /  .'.  \\ \\-'\\   /   \\  `-' /        \\       / |  |'->  `|  |_)  |  |'->\\-'\\   /    |  `---.,------.)\\  '-'(_ .'\n");
    printf("`--'   '--'    `-'     `----'          `-----'  `--'      `--'    `--'       `-'     `------'`------'  `-----'   \n");
*/
}
#else
void print_logo() {
/*
    printf("  (`-.            (`-.                            .-')       ('-.    _   .-')\n");
    printf(" ( OO ).        _(OO  )_                        .(  OO)    _(  OO)  ( '.( OO )_ \n");
    printf("(_/.  \\_)-. ,--(_/   ,. \\  ,--.                (_)---\\_)  (,------.  ,--.   ,--.) ,--. ,--.  \n");
    printf(" \\  `.'  /  \\   \\   /(__/ /  .'       .-')     '  .-.  '   |  .---'  |   `.'   |  |  | |  |   \n");
    printf("  \\     /\\   \\   \\ /   / .  / -.    _(  OO)   ,|  | |  |   |  |      |         |  |  | | .-')\n");
    printf("   \\   \\ |    \\   '   /, | .-.  '  (,------. (_|  | |  |  (|  '--.   |  |'.'|  |  |  |_|( OO )\n");
    printf("  .'    \\_)    \\     /__)' \\  |  |  '------'   |  | |  |   |  .--'   |  |   |  |  |  | | `-' /\n");
    printf(" /  .'.  \\      \\   /    \\  `'  /              '  '-'  '-. |  `---.  |  |   |  | ('  '-'(_.-'\n");
    printf("'--'   '--'      `-'      `----'                `-----'--' `------'  `--'   `--'   `-----'\n");
*/
}
#endif
