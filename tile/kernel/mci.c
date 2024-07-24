#include <kernel/mci.h>

volatile struct mci_registers* mci = (volatile struct mci_registers*)(SMC_CS3_PADDR + 0x50000);

void mci_init() {
  mci->power = MCI_POWER_CTRL;
}

void mci_send_command(uint32_t cmd_index, uint32_t cmd_type, uint32_t cmd_arg) {
  uint32_t command = 0;

  command |= cmd_index;
  command |= cmd_type;
  mci->argument = cmd_arg;
  mci->command = command;
}
