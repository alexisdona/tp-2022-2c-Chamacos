#include "./registers.c"
#include "./segment_table.c"

struct PCB {
  int pid;
  int ppid;
  int *program_counter;
  struct Registers registers;
  struct Segment_Table_Element segment_table_data; // TODO: IMPLEMENTAR
};

void dispatch_pcb(struct PCB *pcb, int dispatch_socket){
  
} 
