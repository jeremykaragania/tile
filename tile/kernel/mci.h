#ifndef MCI_H
#define MCI_H

#include <kernel/asm/memory.h>
#include <stdint.h>

extern volatile struct mci_registers* mci;

struct mci_registers {
  uint32_t power;
  uint32_t clock;
  uint32_t argument;
  uint32_t command;
  uint32_t resp_cmd;
  uint32_t response_0;
  uint32_t response_1;
  uint32_t response_2;
  uint32_t response_3;
  uint32_t data_timer;
  uint32_t data_length;
  uint32_t data_ctrl;
  uint32_t data_cnt;
  uint32_t status;
  uint32_t clear;
  uint32_t mask_0;
  uint32_t mask_1;
  uint32_t select;
  uint32_t fifo_cnt;
  uint32_t reserved_0[12];
  uint32_t fifo;
  uint32_t periph_id_0;
  uint32_t periph_id_1;
  uint32_t periph_id_2;
  uint32_t periph_id_3;
  uint32_t p_cell_d_0;
  uint32_t p_cell_d_1;
  uint32_t p_cell_d_2;
  uint32_t p_cell_d_3;
};

#endif
