#include <drivers/gic_400.h>

volatile struct gic_distributor_registers* gicd = (volatile struct gic_distributor_registers*)GICD_PADDR;
volatile struct gic_cpu_interface_registers* gicc = (volatile struct gic_cpu_interface_registers*)GICC_PADDR;

/*
  gic_init initializes the GIC.
*/
void gic_init() {
  /* Enable group 0 and group 1 interrupts. */
  gicd->ctl = 3;

  /* Enable group 0 and group 1 interrupts. */
  gicc->ctl = 3;

  /* Set the priority mask to lowest priority. */
  gicc->pm = 0xff;
}
