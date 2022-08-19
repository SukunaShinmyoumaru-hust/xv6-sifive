// #define K210

// k210 peripherals
// (0x0200_0000, 0x1000),      /* CLINT     */
// // we only need claim/complete for target0 after initializing
// (0x0C20_0000, 0x1000),      /* PLIC      */
// (0x3800_0000, 0x1000),      /* UARTHS    */
// (0x3800_1000, 0x1000),      /* GPIOHS    */
// (0x5020_0000, 0x1000),      /* GPIO      */
// (0x5024_0000, 0x1000),      /* SPI_SLAVE */
// (0x502B_0000, 0x1000),      /* FPIOA     */
// (0x502D_0000, 0x1000),      /* TIMER0    */
// (0x502E_0000, 0x1000),      /* TIMER1    */
// (0x502F_0000, 0x1000),      /* TIMER2    */
// (0x5044_0000, 0x1000),      /* SYSCTL    */
// (0x5200_0000, 0x1000),      /* SPI0      */
// (0x5300_0000, 0x1000),      /* SPI1      */
// (0x5400_0000, 0x1000),      /* SPI2      */
// (0x8000_0000, 0x600000),    /* Memory    */

// qemu -machine virt is set up like this,
// based on qemu's hw/riscv/virt.c:
//
// 00001000 -- boot ROM, provided by qemu
// 02000000 -- CLINT
// 0C000000 -- PLIC
// 10000000 -- uart0 
// 10001000 -- virtio disk 
// 80000000 -- boot ROM jumps here in machine mode
//             -kernel loads the kernel here
// unused RAM after 80000000.
#include"param.h"
#define VIRT_OFFSET             0x3F00000000L
#define LINKER_OFFSET             0xF00000L

#define SPI2_CTRL_ADDR_P		0x10050000

// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80000000 to PHYSTOP.
#define KERNBASE 0x80200000ULL
#define PHYSTOP (0x80000000ULL + (unsigned long long)(1ULL * 128 * 1024 * 1024)) // 128MB

// map the trampoline page to the highest address,
// in both user and kernel space.

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
// the physical address of rustsbi
#define RUSTSBI_BASE            0x80000000
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))  // 256 GB
#define MAXUVA                  RUSTSBI_BASE

#define USER_TOP (MAXVA)    // virtual address
#define TRAMPOLINE (USER_TOP - PGSIZE)  // virtual address
#define SIG_TRAMPOLINE 	(TRAMPOLINE - PGSIZE)

// qemu puts UART registers here in physical memory.
#define UART0 0x10000000L   // 256 MB
#define UART0_V                  (UART0 + VIRT_OFFSET)

// local interrupt controller, which contains the timer.
#define CLINT                   0x02000000L
#define CLINT_V                 (CLINT + VIRT_OFFSET)
#define CLINT_VMTIME             (CLINT_V + 0xBFF8)

// virtio mmio interface
#define VIRTIO0 0x10001000
#define VIRTIO0_V               (VIRTIO0 + VIRT_OFFSET)

// qemu puts platform-level interrupt controller (PLIC) here.
#define PLIC 0x0c000000L       // 192 MB
#define PLIC_V                  (PLIC + VIRT_OFFSET)
#define PLIC_PRIORITY (PLIC_V + 0x0)
#define PLIC_PENDING (PLIC_V + 0x1000)
#define PLIC_MENABLE(hart) (PLIC_V + 0x1f80 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC_V + 0x2000 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC_V + 0x1ff000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC_V + 0x200000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC_V + 0x1ff004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC_V + 0x200004 + (hart)*0x2000)

#define TRAPFRAME 	(MAXUVA - 2 * PGSIZE) // virtual address
#define USER_STACK_BOTTOM (MAXUVA - (3*PGSIZE))   // stack lower address 
#define USER_MMAP_START   (USER_STACK_BOTTOM - 0x10000000)
#define USER_STACK_TOP    (USER_MMAP_START + PGSIZE)  
#define USER_TEXT_START   0x1000
#define VKSTACK           0x3EC0000000L





