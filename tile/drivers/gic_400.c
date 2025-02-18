#include <drivers/gic_400.h>

volatile struct gic_distributor_registers* gicd = (volatile struct gic_distributor_registers*)(GIC_400_PADDR + 0x1000);
volatile struct gic_cpu_interface_registers* gicc = (volatile struct gic_cpu_interface_registers*)(GIC_400_PADDR + 0x2000);
