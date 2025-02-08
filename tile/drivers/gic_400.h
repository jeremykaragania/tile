#ifndef GIC_400
#define GIC_400

#include <stdint.h>

struct gic_distributor_registers {
  uint32_t ctlr;
  const uint32_t type;
  const uint32_t iid;
  const uint32_t reserved_0[30];
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
  uint32_t icactive[15];
  const uint32_t reserved_6[17];
  uint32_t ipriority[127];
  const uint32_t reserved_7[129];
  uint32_t itargets[127];
  const uint32_t reserved_8[129];
  uint32_t icfg[31];
  const uint32_t reserved_9[33];
  uint32_t ppis;
  uint32_t spis[14];
  const uint32_t reserved_10[113];
  uint32_t sgi;
  const uint32_t reserved_11[4];
  uint32_t cpendsgi[3];
  const uint32_t reserved_12;
  uint32_t spendsgi[3];
  const uint32_t reserved_13[41];
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

#endif
