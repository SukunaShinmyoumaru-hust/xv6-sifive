#ifndef __PRINTF_H
#define __PRINTF_H
#define QEMU_LOGO1 "  (`-.            (`-.                            .-')       ('-.    _   .-')\n"
#define QEMU_LOGO2 " ( OO ).        _(OO  )_                        .(  OO)    _(  OO)  ( '.( OO )_ \n"
#define QEMU_LOGO3 "(_/.  \\_)-. ,--(_/   ,. \\  ,--.                (_)---\\_)  (,------.  ,--.   ,--.) ,--. ,--.  \n"
#define QEMU_LOGO4 " \\  `.'  /  \\   \\   /(__/ /  .'       .-')     '  .-.  '   |  .---'  |   `.'   |  |  | |  |   \n"
#define QEMU_LOGO5 "  \\     /\\   \\   \\ /   / .  / -.    _(  OO)   ,|  | |  |   |  |      |         |  |  | | .-')\n"
#define QEMU_LOGO6 "   \\   \\ |    \\   '   /, | .-.  '  (,------. (_|  | |  |  (|  '--.   |  |'.'|  |  |  |_|( OO )\n"
#define QEMU_LOGO7 "  .'    \\_)    \\     /__)' \\  |  |  '------'   |  | |  |   |  .--'   |  |   |  |  |  | | `-' /\n"
#define QEMU_LOGO8 " /  .'.  \\      \\   /    \\  `'  /              '  '-'  '-. |  `---.  |  |   |  | ('  '-'(_.-'\n"
#define QEMU_LOGO9 "'--'   '--'      `-'      `----'                `-----'--' `------'  `--'   `--'   `-----'\n"
#define BACKSPACE 0x100


void consputc(int c);

void printfinit(void);

void printf(char *fmt, ...);

void panic(char *s) __attribute__((noreturn));

void backtrace();

void print_logo();

void __debug_info(char *fmt, ...);
void __debug_warn(char *fmt, ...);
void __debug_error(char *fmt, ...);

#endif 
