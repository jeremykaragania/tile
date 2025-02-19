#include <drivers/gic_400.h>

volatile struct gic_distributor_registers* gicd = (volatile struct gic_distributor_registers*)GICD_PADDR;
volatile struct gic_cpu_interface_registers* gicc = (volatile struct gic_cpu_interface_registers*)GICC_PADDR;
