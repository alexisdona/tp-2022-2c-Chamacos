#ifndef TP_2022_2C_CHAMACOS_CPU_H
#define TP_2022_2C_CHAMACOS_CPU_H

#include "../../shared/headers/shared.h"

#define LOG_FILE "cpu.log"
#define LOG_NAME "cpu_log"

t_config* cpu_config;
t_config* communication_config;
t_log* logger;

char** lista_dispositivos;

uint32_t hubo_interrupcion;
uint32_t retardo_operacion_cpu;
t_pcb* pcb;
op_code estado_proceso;

pthread_t thread_escucha_memoria;
pthread_t thread_escucha_dispatch;
pthread_t thread_escucha_interrupt;

pthread_mutex_t mutex_flag_interrupcion;

//Semaforos de sincronizacion entre hilos de cpu
sem_t desalojar_pcb;
sem_t continuar_ciclo_instruccion;

//Conexiones a otros modulos
void* conexion_dispatch(void* socket);
void* conexion_interrupt(void* socket);
void* conexion_memoria(void* socket);

//Funciones del ciclo de instruccion
void comenzar_ciclo_instruccion();
t_instruccion* fase_fetch();
int fase_decode(t_instruccion* instruccion);
operando fase_fetch_operand(operando direccion_operador_a_buscar);
op_code fase_execute(t_instruccion* instruccion, uint32_t operador);
op_code operacion_SET(registro_cpu registro,uint32_t valor);
op_code operacion_ADD(registro_cpu,registro_cpu);
op_code operacion_MOV_IN(registro_cpu*,uint32_t direccion);
op_code operacion_MOV_OUT(uint32_t direccion,registro_cpu);
op_code operacion_IO(dispositivo,uint32_t unidades_trabajo);
op_code operacion_EXIT();
void chequear_interrupcion();
void desalojo_proceso();

#endif //TP_2022_2C_CHAMACOS_CPU_H
