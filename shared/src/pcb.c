#include "headers/pcb.h"

void dispatch_pcb(t_pcb* pcb, int dispatch_socket){
  t_paquete* package = crear_paquete();
  package->codigo_operacion = PCB;
  agregar_entero(package, pcb->pid);
  agregar_entero(package, pcb->ppid);
  agregar_entero(package, pcb->program_counter);
  agregar_entero(package, pcb->registros_pcb.registro_ax);
  agregar_entero(package, pcb->registros_pcb.registro_bx);
  agregar_entero(package, pcb->registros_pcb.registro_cx);
  agregar_entero(package, pcb->registros_pcb.registro_dx);
  agregar_entero(package, pcb->datos_segmentos.indice_tabla_paginas_segmentos); //TODO: revisar

   enviar_paquete(package, dispatch_socket);
} 
