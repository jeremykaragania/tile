#include <kernel/mci.h>

volatile struct mci_registers* mci = (volatile struct mci_registers*)(SMC_CS3_PADDR + 0x50000);

/*
  mci_init initializes the multimedia card interface.
*/
void mci_init() {
  mci_send_command(0, MCI_COMMAND_ENABLE, 0);
  mci_send_command(55, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, 0);
  mci_send_command(41, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, 0x7fffff);
  mci_send_command(2, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE | MCI_COMMAND_LONG_RSP, 0);
  mci_send_command(3, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, 0);
  mci_send_command(7, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, mci->response[0]);
  mci_send_command(16, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, 1);
}

/*
  mci_getchar reads a byte from the SD card at the address "addr". It returns
  the read byte.
*/
char mci_getchar(uint32_t addr) {
  mci->data_length = 1;
  mci->data_ctrl |= 0x3;
  mci_send_command(17, MCI_COMMAND_ENABLE | MCI_COMMAND_RESPONSE, addr);
  while (!(mci->status & MCI_STATUS_DATA_BLOCK_END));
  return ((char*)mci->fifo)[0];
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
    return 0;
  }

  command |= cmd_index;
  command |= cmd_type;
  mci->argument = cmd_arg;
  mci->command = command;

  return 1;
}
