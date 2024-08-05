#ifndef MCI_H
#define MCI_H

#include <kernel/asm/memory.h>
#include <stdint.h>

#define MCI_COMMAND_RESPONSE 1 << 6
#define MCI_COMMAND_LONG_RSP 1 << 7
#define MCI_COMMAND_INTERRUPT 1 << 8
#define MCI_COMMAND_PENDING 1 << 9
#define MCI_COMMAND_ENABLE 1 << 10

extern volatile struct mci_registers* mci;

struct mci_registers {
  uint32_t power;
  uint32_t clock;
  uint32_t argument;
  uint32_t command;
  uint32_t resp_cmd;
  uint32_t response[4];
  uint32_t data_timer;
  uint32_t data_length;
  uint32_t data_ctrl;
  uint32_t data_cnt;
  uint32_t status;
  uint32_t clear;
  uint32_t mask[2];
  uint32_t select;
  uint32_t fifo_cnt;
  uint32_t reserved_0[13];
  uint32_t fifo[16];
  uint32_t reserved_1[968];
  uint32_t periph_id[4];
  uint32_t p_cell_d[4];
};

void mci_init();

int mci_send_command(uint32_t cmd_index, uint32_t cmd_type, uint32_t cmd_arg);

#endif
