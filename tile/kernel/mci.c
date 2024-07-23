#include <kernel/mci.h>

volatile struct mci_registers* mci = (volatile struct mci_registers*)(SMC_CS3_PADDR + 0x50000);

void mci_init() {
  mci->power = MCI_POWER_CTRL;
}
