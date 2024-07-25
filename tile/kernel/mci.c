#include <kernel/mci.h>

volatile struct mci_registers* mci = (volatile struct mci_registers*)(SMC_CS3_PADDR + 0x50000);

/*
  mci_init initializes the multimedia card interface.
*/
void mci_init() {
  mci->power = MCI_POWER_CTRL;
}

/*
  mci_send_command sends a command to the multimedia card interface. A command
  contains a command index "cmd_index" which specifies which command to
  perform, command type "cmd_type" which specifies the operation of the command
  path state machine, and a command argument "cmd_arg" which specifies an
  argument for the command.
*/
void mci_send_command(uint32_t cmd_index, uint32_t cmd_type, uint32_t cmd_arg) {
  uint32_t command = 0;

  command |= cmd_index;
  command |= cmd_type;
  mci->argument = cmd_arg;
  mci->command = command;
}
