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
    t_list* segmentos;
} t_segmentos;

typedef struct {
  uint32_t pid;
  uint32_t program_counter;
  t_registros_pcb registros_pcb;
  t_list* lista_instrucciones;
  t_segmentos datos_segmentos;
  uint32_t socket_consola;
} t_pcb;


#endif //TP_2022_2C_GECK_PCB_H

