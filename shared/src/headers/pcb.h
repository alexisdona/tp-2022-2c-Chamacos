#ifndef TP_2022_2C_GECK_PCB_H
#define TP_2022_2C_GECK_PCB_H

#include "./../registers.c"
#include "./../segment_table.c"
#include "./../communication.c"
#include "shared.h"

struct PCB {
  uint32_t pid;
  uint32_t ppid;
  uint32_t program_counter;
  struct Registers registers;
  struct Segment_Table_Element segment_table_data; // TODO: IMPLEMENTAR
};

void dispatch_pcb(struct PCB *pcb, int dispatch_socket);

#endif //TP_2022_2C_GECK_PCB_H

