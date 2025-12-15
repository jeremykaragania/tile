/*
  gic_400.c provides a CoreLink GIC-400 Generic Interrupt Controller driver.
*/

#include <drivers/gic_400.h>
#include <kernel/interrupts.h>

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

  /* Ensure that each interrupt has a distinct priority. */
  gic_set_interrupt_priority(TIM01INT, 0);
  gic_set_interrupt_priority(UART0INTR, 1);

  /* Set the priority mask to lowest priority. */
  gicc->pm = 0xff;

  /* The number of interrupts that the GIC supports. */
  gicd_it_lines_number = (gicd->type & 0x1f) + 1;

  /* Enable all interrupts. */
  for (size_t i = 0; i < gicd_it_lines_number; ++i) {
    gicd->isenable[i] = 0xffffffff;
  }
}

/*
  gic_get_interrupt_priority returns the prioritiy for the interrupt with ID
  "id".
*/
uint8_t gic_get_interrupt_priority(uint32_t id) {
  uint32_t n = id >> 2;

  return ((uint8_t*)(gicd->ipriority + n))[id % 4];
}

/*
  gic_set_interrupt_priority sets the prioritiy for the interrupt ID "id" to
  "priority".
*/
void gic_set_interrupt_priority(uint32_t id, uint8_t priority) {
  uint32_t n = id >> 2;

  ((uint8_t*)(gicd->ipriority + n))[id % 4] = priority;
}

/*
  gic_disable_interrupt disables the interrupt with ID "id".
*/
void gic_disable_interrupt(uint32_t id) {
  uint32_t n = id >> 5;

  gicd->icenable[n] = 1 << (id % 32);
}

/*
  gic_enable_interrupt enables the interrupt with ID "id".
*/
void gic_enable_interrupt(uint32_t id) {
  uint32_t n = id >> 5;

  gicd->isenable[n] = 1 << (id % 32);
}
