#ifndef TP_2022_2C_GECK_PCB_H
#define TP_2022_2C_GECK_PCB_H

#include "shared.h"

typedef struct {
    uint32_t registro_ax;
    uint32_t registro_bx;
    uint32_t registro_cx;
    uint32_t registro_dx;
} t_registros_pcb;

typedef struct {
    uint32_t indice_tabla_paginas_segmentos;
   // uint32_t* segmentos[3]; //TODO revisar
} t_segmentos;

typedef struct {
  uint32_t pid;
  uint32_t ppid;
  uint32_t program_counter;
  t_registros_pcb registros_pcb;
  t_segmentos datos_segmentos; // TODO: IMPLEMENTAR
} t_pcb;



void dispatch_pcb(t_pcb* , int dispatch_socket);

#endif //TP_2022_2C_GECK_PCB_H

