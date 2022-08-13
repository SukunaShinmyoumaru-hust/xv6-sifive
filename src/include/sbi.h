#ifndef __SBI_H
#define __SBI_H
#include"types.h"

#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI 3
#define SBI_SEND_IPI 4
#define SBI_REMOTE_FENCE_I 5
#define SBI_REMOTE_SFENCE_VMA 6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN 8

struct sbiret {
	long error;
	long value;
};

enum sbi_ext_hsm_fid {
	SBI_EXT_HSM_HART_START = 0,
	SBI_EXT_HSM_HART_STOP,
	SBI_EXT_HSM_HART_STATUS,
};

static int inline sbi_call(uint64 which, uint64 arg0, uint64 arg1, uint64 arg2) {
    register uint64 a0 asm("a0") = arg0;
    register uint64 a1 asm("a1") = arg1;
    register uint64 a2 asm("a2") = arg2;
    register uint64 a7 asm("a7") = which;
    asm volatile("ecall"
                 : "=r"(a0)
                 : "r"(a0), "r"(a1), "r"(a2), "r"(a7)
                 : "memory");
    return a0;
}

static inline struct sbiret a_sbi_ecall(int ext, int fid, unsigned long arg0,
			unsigned long arg1, unsigned long arg2,
			unsigned long arg3, unsigned long arg4,
			unsigned long arg5)
{
	struct sbiret ret;

	register uint64 a0 asm ("a0") = (uint64)(arg0);
	register uint64 a1 asm ("a1") = (uint64)(arg1);
	register uint64 a2 asm ("a2") = (uint64)(arg2);
	register uint64 a3 asm ("a3") = (uint64)(arg3);
	register uint64 a4 asm ("a4") = (uint64)(arg4);
	register uint64 a5 asm ("a5") = (uint64)(arg5);
	register uint64 a6 asm ("a6") = (uint64)(fid);
	register uint64 a7 asm ("a7") = (uint64)(ext);
	asm volatile ("ecall"
		      : "+r" (a0), "+r" (a1)
		      : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
		      : "memory");
	ret.error = a0;
	ret.value = a1;

	return ret;
}

static inline void sbi_console_putchar(int c) {
    sbi_call(SBI_CONSOLE_PUTCHAR, c, 0, 0);
}

static inline int sbi_console_getchar() {
    return sbi_call(SBI_CONSOLE_GETCHAR, 0, 0, 0);
}

static inline void shutdown() {
    sbi_call(SBI_SHUTDOWN, 0, 0, 0);
    //panic("shutdown");
}

static inline void sbi_set_mie(void) {
	sbi_call(0x0A000005, 0, 0, 0);
}
/*
static inline void set_timer(uint64 stime) {
    sbi_call(SBI_SET_TIMER, stime, 0, 0);
}
*/

// Time Extension 

#define TIME_EID 	0x54494d45
#define TIME_SET_TIMER 	0

static inline struct sbiret set_timer(uint64 stime_value) {
	return a_sbi_ecall(TIME_EID, TIME_SET_TIMER, stime_value, 0, 0, 0, 0, 0);
}

static inline void start_hart(uint64 hartid,uint64 start_addr, uint64 a1) {
    a_sbi_ecall(0x48534D, 0, hartid, start_addr, a1, 0, 0, 0);
}

static inline struct sbiret get_state(uint64 hartid) {
    return a_sbi_ecall(0x48534D, 2, hartid, 0, 0, 0, 0, 0);
}

static inline void send_ipi(uint64 mask) {
    a_sbi_ecall(0x735049, 0, mask,0,0,0,0,0);
}

static inline int sbi_hsm_hart_status(unsigned long hart){
    struct sbiret ret;
    ret = a_sbi_ecall(0x48534D, 2, hart, 0, 0, 0, 0, 0);
    return (ret.error != 0 ? (int)ret.error : (int)ret.value);
}

#endif
