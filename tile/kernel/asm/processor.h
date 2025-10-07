#ifndef ASM_PROCESSOR_H
#define ASM_PROCESSOR_H

/* Processor modes. */
#define PM_USR 0x10
#define PM_FIQ 0x11
#define PM_IRQ 0x12
#define PM_SVC 0x13
#define PM_MON 0x16
#define PM_ABT 0x17
#define PM_HYP 0x1a
#define PM_UND 0x1b
#define PM_SYS 0x1f

/* Processor register indexes. */
#define PR_R0 0
#define PR_R1 1
#define PR_R2 2
#define PR_R3 3
#define PR_R4 4
#define PR_R5 5
#define PR_R6 6
#define PR_R7 7
#define PR_R8 8
#define PR_R9 9
#define PR_R10 10
#define PR_R11 11
#define PR_R12 12
#define PR_SP 13
#define PR_LR 14
#define PR_PC 15
#define PR_CPSR 16

#endif
