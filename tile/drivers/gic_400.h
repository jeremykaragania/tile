#ifndef GIC_400
#define GIC_400

#include <kernel/asm/memory.h>
#include <stddef.h>
#include <stdint.h>

/*
  struct gic_distributor_registers represents the registers of the distributor
  of the GIC-400.
*/
struct gic_distributor_registers {
  uint32_t ctl;
  const uint32_t type;
  const uint32_t iid;
  const uint32_t reserved_0[29];
  uint32_t igroup[15];
  const uint32_t reserved_1[17];
  uint32_t isenable[15];
  const uint32_t reserved_2[17];
  uint32_t icenable[15];
  const uint32_t reserved_3[17];
  uint32_t ispend[15];
  const uint32_t reserved_4[17];
  uint32_t icpend[15];
  const uint32_t reserved_5[17];
  uint32_t isactive[15];
  const uint32_t reserved_6[17];
  uint32_t icactive[15];
  const uint32_t reserved_7[17];
  uint32_t ipriority[127];
  const uint32_t reserved_8[129];
  uint32_t itargets[127];
  const uint32_t reserved_9[129];
  uint32_t icfg[31];
  const uint32_t reserved_10[33];
  uint32_t ppis;
  uint32_t spis[14];
  const uint32_t reserved_11[113];
  uint32_t sgi;
  const uint32_t reserved_12[3];
  uint32_t cpendsgi[3];
  const uint32_t reserved_13;
  uint32_t spendsgi[3];
  const uint32_t reserved_14[41];
  uint32_t pid4;
  uint32_t pid5;
  uint32_t pid6;
  uint32_t pid7;
  uint32_t pid0;
  uint32_t pid1;
  uint32_t pid2;
  uint32_t pid3;
  uint32_t cid0;
  uint32_t cid1;
  uint32_t cid2;
  uint32_t cid3;
};

/*
  struct gic_cpu_interface_registers represents the registers of the CPU
  interface of the GIC-400.
*/
struct gic_cpu_interface_registers {
  uint32_t ctl;
  uint32_t pm;
  uint32_t bp;
  uint32_t ia;
  uint32_t eoi;
  uint32_t rp;
  uint32_t hppi;
  uint32_t abp;
  uint32_t aia;
  uint32_t aeoi;
  uint32_t ahppi;
  uint32_t reserved_0[41];
  uint32_t ap;
  uint32_t reserved_[3];
  uint32_t nsap;
  uint32_t reserved_2[6];
  uint32_t iid;
  uint32_t reserved_3[960];
  uint32_t di;
};

extern volatile struct gic_distributor_registers* gicd;
extern volatile struct gic_cpu_interface_registers* gicc;
extern uint32_t gicd_it_lines_number;

void gic_init();

void gic_disable_interrupt(uint32_t id);
void gic_enable_interrupt(uint32_t id);

#endif
