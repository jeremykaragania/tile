/*
  pl180.c provides an ARM PrimeCell Multimedia Card Interface (PL180) driver.
*/

#include <drivers/pl180.h>

volatile struct mci_registers* mci = (volatile struct mci_registers*)MCI_PADDR;

/*
  mci_init initializes the multimedia card interface.
*/
void mci_init() {
  /* GO_IDLE_STATE */
  mci_send_command(0, MCI_COMMAND_ENABLE, 0);

  /* APP_CMD */
  mci_send_command(55, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, 0);

  /* SD_SEND_OP_COND */
  mci_send_command(41, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, 0x7fffff);

  /* ALL_SEND_CID */
  mci_send_command(2, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE | MCI_COMMAND_LONG_RSP, 0);

  /* SEND_RELATIVE_ADDR */
  mci_send_command(3, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, 0);

  /* SELECT/DESELECT_CARD */
  mci_send_command(7, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, mci->response[0]);

  /* SET_BLOCKLEN */
  mci_send_command(16, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, MCI_BLOCK_SIZE);
}

/*
  mci_read reads MCI_BLOCK_SIZE bytes from the SD card at the address "addr"
  and stores them in the buffer "buf". The number of bytes read is returned.
*/
size_t mci_read(uint32_t addr, void* buf) {
  /* READ_SINGLE_BLOCK */
  mci_send_command(17, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, addr);
  mci->data_length = MCI_BLOCK_SIZE;
  mci->data_ctrl = 0x3;

  for (size_t i = 0; i < MCI_BLOCK_SIZE / 4; ++i) {
    while (!(mci->status & MCI_STATUS_DATA_BLOCK_END));
    ((uint32_t*)buf)[i] = mci->fifo[0];
  }

  return MCI_BLOCK_SIZE;
}

/*
  mci_write writes MCI_BLOCK_SIZE bytes from the buffer "buf" to the SD card
  at the address "addr". The number of bytes written is returned.
*/
size_t mci_write(uint32_t addr, const void* buf) {
  /* WRITE_BLOCK */
  mci_send_command(24, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, addr);
  mci->data_length = MCI_BLOCK_SIZE;
  mci->data_ctrl = 0x1;

  for (size_t i = 0; i < MCI_BLOCK_SIZE / 4; ++i) {
    while (!(mci->status & MCI_STATUS_DATA_BLOCK_END));
    mci->fifo[0] = ((uint32_t*)buf)[i];
  }

  return MCI_BLOCK_SIZE;
}

/*
  mci_send_command sends a command to the multimedia card interface. A command
  contains a command index "cmd_index" which specifies which command to
  perform, command type "cmd_type" which specifies the operation of the command
  path state machine, and a command argument "cmd_arg" which specifies an
  argument for the command.
*/
int mci_send_command(uint32_t cmd_index, uint32_t cmd_type, uint32_t cmd_arg) {
  uint32_t command = 0;

  if (cmd_index > 63) {
    return -1;
  }

  command |= cmd_index;
  command |= cmd_type;
  mci->argument = cmd_arg;
  mci->command = command;

  return 0;
}
