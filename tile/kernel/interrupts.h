#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include <kernel/processor.h>

// Watchdog timer.
#define WDOG0INT 32

// Software interrupts.
#define SWINT 33

// Timer interrupts.
#define TIM01INT 34
#define TIM23INT 35
#define RTCINTR 36

// UART interrupts.
#define UART0INTR 37
#define UART1INTR 38
#define UART2INTR 39
#define UART3INTR 40

// MultiMedia card interrupts.
#define MCI_INTR_0 41
#define MCI_INTR_1 42

// Audio CODEC interrupts.
#define AACI_INTR 43

// Keyboard/Mouse interrupts.
#define KMI0_INTR 44
#define KMI1_INTR 45

// Display interrupts.
#define CLCDINTR 46

// Ethernet interrupts.
#define ETH_INTR 47

// USB interrupts.
#define USB_INT 48

// PCI-Express interrupts.
#define PCIE_GPEN 49

int handle_fault(uint32_t addr);

void do_reset();
void do_undefined_instruction();
void do_supervisor_call();
void do_prefetch_abort();
void do_data_abort();
void do_irq();
void do_fiq();

extern uint32_t get_dfar();
extern uint32_t get_ifar();
extern void ret_from_interrupt(struct processor_registers* registers);
extern void ret_from_interrupt_user();

#endif
