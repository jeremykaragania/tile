#include <drivers/gic_400.h>

volatile struct gic_distributor_registers* gicd = (volatile struct gic_distributor_registers*)GICD_PADDR;
volatile struct gic_cpu_interface_registers* gicc = (volatile struct gic_cpu_interface_registers*)GICC_PADDR;
uint32_t gicd_it_lines_number;

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

  /* The number of interrupts that the GIC supports. */
  gicd_it_lines_number = (gicd->type & 0x1f) + 1;

  /* Enable all interrupts. */
  for (size_t i = 0; i < gicd_it_lines_number; ++i) {
    gicd->isenable[i] = 0xffffffff;
  }
}

void gic_disable_interrupt(uint32_t id) {
  uint32_t n = id / 32;

  gicd->icenable[n] |= 1 << (id % 32);
}

void gic_enable_interrupt(uint32_t id) {
  uint32_t n = id / 32;

  gicd->isenable[n] |= 1 << (id % 32);
}
