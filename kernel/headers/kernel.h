#ifndef TP_2022_2C_CHAMACOS_KERNEL_H
#define TP_2022_2C_CHAMACOS_KERNEL_H

#include "../../shared/headers/shared.h"

#define LOG_FILE "kernel.log"
#define LOG_NAME "kernel_log"

uint32_t ultimo_pid;

t_config* kernel_config;
t_config* communication_config;
t_log* logger;

pthread_t thread_escucha_memoria;
pthread_t thread_escucha_dispatch;
pthread_t thread_escucha_interrupt;

//Semaforos de sincronizacion
sem_t grado_multiprogramacion;
sem_t new_process;
sem_t new_to_ready;
sem_t ready_to_running;
sem_t cpu_libre;
sem_t finish_process;

//Mutex para proteger las colas
pthread_mutex_t mutex_new;
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_running;
pthread_mutex_t mutex_blocked_screen;
pthread_mutex_t mutex_blocked_keyboard;
pthread_mutex_t mutex_blocked_page_fault;
pthread_mutex_t mutex_blocked_io;
pthread_mutex_t mutex_exit;

//Sockets de modulos
int socket_dispatch;
int socket_interrupt;
int socket_memoria;

//Mutex para proteccion de sockets
pthread_mutex_t mutex_dispatch;
pthread_mutex_t mutex_interrupt;
pthread_mutex_t mutex_memoria;

t_queue* new_queue;
t_queue* exit_queue;
t_queue* ready_queue;
t_queue* blocked_screen_queue;
t_queue* blocked_keyboard_queue;
t_queue* blocked_page_fault_queue;
t_queue* blocked_io_queue;
t_queue* running_queue;
t_queue* exit_queue;

//Obtiene el ip y puerto del kernel para iniciar el servidor, devuele el socket
int levantar_servidor();

void esperar_modulos(int socket_srv);
void esperar_consolas(int socket_srv);

void *conexion_consola(void* socket_consola);
void *conexion_dispatch(void* socket_dispatch);
void *conexion_interrupt(void* socket_interrupt);
void *conexion_memoria(void* socket_memoria);
t_pcb* crear_estructura_pcb(t_list* lista_instrucciones, t_list* segmentos);

// Funciones de planificacion ===============================

//Inicio de planificacion y planificadores
void iniciar_planificacion();
void* planificador_largo_plazo(void*);
void* planificador_corto_plazo(void*);

void inicializar_mutex();
void inicializar_semaforos_sincronizacion(uint32_t grado_multiprogramacion);

void agregar_pcb_a_cola(t_pcb*,pthread_mutex_t, t_queue*);
t_pcb* quitar_pcb_de_cola(pthread_mutex_t, t_queue* cola);

//Funciones del planificador de largo plazo =================

//Espera para poder agregar un pcb de new a ready
void pcb_a_dispatcher();

//Espera para finalizar un pcb
void* finalizador_procesos(void*);

//Funciones del planificador de corto plazo =================

void* manejador_estado_ready(void*);
void* manejador_estado_running(void*);

#endif //TP_2022_2C_CHAMACOS_KERNEL_H