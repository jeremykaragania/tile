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

/* Processor register offsets. */
#define PR_R0_OFFSET 0
#define PR_R1_OFFSET 4
#define PR_R2_OFFSET 8
#define PR_R3_OFFSET 12
#define PR_R4_OFFSET 16
#define PR_R5_OFFSET 20
#define PR_R6_OFFSET 24
#define PR_R7_OFFSET 28
#define PR_R8_OFFSET 32
#define PR_R9_OFFSET 36
#define PR_R10_OFFSET 40
#define PR_R11_OFFSET 44
#define PR_R12_OFFSET 48
#define PR_SP_OFFSET 52
#define PR_LR_OFFSET 56
#define PR_PC_OFFSET 60
#define PR_CPSR_OFFSET 64

#endif
