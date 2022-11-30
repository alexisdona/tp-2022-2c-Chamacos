#ifndef TP_2022_2C_CHAMACOS_CPU_H
#define TP_2022_2C_CHAMACOS_CPU_H

#include "../../shared/headers/shared.h"
#include <math.h>

#define LOG_FILE "cpu.log"
#define LOG_NAME "cpu_log"

typedef struct
{
    uint32_t marco;
    uint32_t desplazamiento;

} dir_fisica;

typedef struct
{
    uint32_t pid;
    uint32_t numero_segmento;
    uint32_t indice_tabla_paginas;
    uint32_t numero_pagina;
    dir_fisica * direccion_fisica;
} punteros_cpu;

typedef struct
{
    uint32_t pid;
    uint32_t segmento;
    uint32_t pagina;
    uint32_t marco;
    uint32_t veces_referenciada;
} tlb_entrada;

int socket_kernel_dispatch, socket_kernel_interrupt, socket_memoria;

uint32_t tamanio_pagina, entradas_por_tabla, entradas_max_tlb;
t_list* tlb;
char* algoritmo_reemplazo_tlb;

t_config* cpu_config;
t_config* communication_config;
t_log* logger;

char** lista_dispositivos;

uint32_t hubo_interrupcion;
uint32_t retardo_operacion_cpu;
t_pcb* pcb;
op_code estado_proceso;

pthread_t thread_escucha_dispatch;
pthread_t thread_escucha_interrupt;

pthread_mutex_t mutex_flag_interrupcion;

//Semaforos de sincronizacion entre hilos de cpu
sem_t desalojar_pcb;
sem_t continuar_ciclo_instruccion;

//Conexiones a otros modulos
void* conexion_dispatch(void* socket);
void* conexion_interrupt(void* socket);

//Funciones del ciclo de instruccion
void comenzar_ciclo_instruccion();
t_instruccion* fase_fetch();
int fase_decode(t_instruccion* instruccion);
operando fase_fetch_operand(operando direccion_operador_a_buscar);
op_code fase_execute(t_instruccion* instruccion, uint32_t operador);
op_code operacion_SET(registro_cpu registro,uint32_t valor);
op_code operacion_ADD(registro_cpu,registro_cpu);
op_code operacion_MOV_IN(registro_cpu,uint32_t direccion);
op_code operacion_MOV_OUT(uint32_t direccion,registro_cpu);
op_code operacion_IO(dispositivo,uint32_t unidades_trabajo);
op_code operacion_EXIT();
void chequear_interrupcion();
void desalojo_proceso();
punteros_cpu * obtener_direccion_fisica(uint32_t direccion_logica);
void handshake_memoria(int conexionMemoria);
int tlb_obtener_marco(uint32_t pid, uint32_t numero_segmento, uint32_t numero_pagina);
void reemplazar_entrada_tlb(tlb_entrada* entrada);
void tlb_actualizar(uint32_t pid, uint32_t numero_segmento, uint32_t numero_pagina, uint32_t marco);
void actualizar_entrada_marco_existente(uint32_t numero_pagina, uint32_t marco);
static bool comparator (void*, void*);
uint32_t leer_en_memoria(punteros_cpu * punteros_cpu);
int obtener_marco_memoria(uint32_t indice_tabla_paginas, uint32_t numero_pagina);
void escribir_en_memoria(punteros_cpu * direccion_fisica, uint32_t valor);

#endif //TP_2022_2C_CHAMACOS_CPU_H