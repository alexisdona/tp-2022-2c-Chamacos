#include "headers/pcb.h"

void dispatch_pcb(struct PCB *pcb, int dispatch_socket){
  t_paquete* package = crear_paquete();
  package->codigo_operacion = PCB;
  agregar_entero(package, pcb->pid);
  agregar_entero(package, pcb->ppid);
  agregar_entero(package, pcb->program_counter);
  agregar_entero(package, pcb->registers.ax);
  agregar_entero(package, pcb->registers.bx);
  agregar_entero(package, pcb->registers.cx);
  agregar_entero(package, pcb->registers.dx);
  agregar_entero(package, pcb->segment_table_data.segment_id); //TODO: revisar
  agregar_entero(package, pcb->segment_table_data.segment_size);
  agregar_entero(package, pcb->segment_table_data.segment_offset);

   enviar_paquete(package, dispatch_socket);
} 
