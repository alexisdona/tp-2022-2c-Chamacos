#ifndef TP_2022_2C_CHAMACOS_PLANIFICATION_H
#define TP_2022_2C_CHAMACOS_PLANIFICATION_H

#include "kernel.h"

//Semaforos de sincronizacion
sem_t grado_multiprogramacion;
sem_t new_process;
sem_t new_to_ready;
sem_t ready_to_running;
sem_t execute_process;
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

t_queue* new_queue;
t_queue* exit_queue;
t_queue* ready_queue;
t_queue* blocked_screen;
t_queue* blocked_keyboard;
t_queue* blocked_page_fault;
t_queue* blocked_io;
t_queue* running_queue;
t_queue* exit;

//Estructura pcb provisoria
typedef struct {
    size_t id_proceso;
    size_t program_counter;
    t_list* lista_instrucciones;
    size_t consola_fd;
    size_t kernel_fd;
} t_pcb;

//Inicio de planificacion y planificadores
void init_planification();
void long_planner();
void dispatcher();

void init_all_mutex();
void init_all_syncro_semaphores(uint32_t grado_multiprogramacion);

void push_pcb_to_queue(t_pcb*,pthread_mutex_t,t_queue*);
t_pcb* pop_pcb_from_queue(pthread_mutex_t mutex, t_queue* cola);

//Funciones del planificador de largo plazo =================

//Espera para poder agregar un pcb de new a ready
void pcb_to_dispatcher();

//Espera para finalizar un pcb
void process_terminator();

//Funciones del planificador de corto plazo =================

//Gestiona el pasaje de ready a running
void ready_handler();

#endif //TP_2022_2C_CHAMACOS_PLANIFICATION_H