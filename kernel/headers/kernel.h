#ifndef TP_2022_2C_CHAMACOS_KERNEL_H
#define TP_2022_2C_CHAMACOS_KERNEL_H

#include "../../shared/headers/shared.h"

#define LOG_FILE "kernel.log"
#define LOG_NAME "kernel_log"

uint32_t cantidad_dispositivos;
uint32_t hay_proceso_ejecutando;
uint32_t INTERRUPCIONES_HABILITADAS;
uint32_t ultimo_pid;
uint32_t quantum;
uint32_t* tiempos_bloqueos;

char* algoritmo_planificacion;
char** lista_dispositivos;

t_config* kernel_config;
t_config* communication_config;
t_log* logger;

pthread_mutex_t mutex_logger;

pthread_t thread_escucha_memoria;
pthread_t thread_escucha_dispatch;
pthread_t thread_escucha_interrupt;
pthread_t thread_clock;

int socket_cpu_dispatch;
int socket_memoria;

//Semaforos de sincronizacion
sem_t grado_multiprogramacion;
sem_t new_process;
sem_t new_to_ready;
sem_t pcbs_en_ready;
sem_t cpu_libre;
sem_t finish_process;
sem_t continuar_conteo_quantum;
sem_t enviar_pcb_a_cpu;
sem_t bloquear_por_pantalla;
sem_t bloquear_por_teclado;
sem_t bloquear_por_pf;
sem_t estructuras_administrativas_pcb_listas;

t_list* semaforos_dispositivos;

//Mutex para proteger las colas
pthread_mutex_t mutex_new;
pthread_mutex_t mutex_ready1;
pthread_mutex_t mutex_ready2;
pthread_mutex_t mutex_running;
pthread_mutex_t mutex_blocked_screen;
pthread_mutex_t mutex_blocked_keyboard;
pthread_mutex_t mutex_blocked_page_fault;
pthread_mutex_t mutex_exit;
pthread_mutex_t mutex_pid;
pthread_mutex_t mutex_blocked_io;

t_queue* new_queue;
t_list* ready1_list;
t_list* ready2_list;
t_list* blocked_io_list;
t_list* blocked_screen_list;
t_list* blocked_keyboard_list;
t_queue* blocked_page_fault_queue;
t_queue* running_queue;
t_queue* exit_queue;

typedef enum {
    NEW,
    READY1,
    READY2,
    RUNNING,
    BLOQUEADO_IO,
    BLOQUEADO_PANTALLA,
    BLOQUEADO_TECLADO,
    BLOQUEADO_PAGE_FAULT,
    EXIT_S
} estado_pcb;

estado_pcb ready_anterior_pcb_running;

//Obtiene el ip y puerto del kernel para iniciar el servidor, devuele el socket
int levantar_servidor();

void esperar_modulos(int socket_srv);
void esperar_consolas(int socket_srv);

void *conexion_consola(void* socket_consola);
void *conexion_dispatch(void* socket_dispatch);
void *conexion_interrupt(void* socket_interrupt);
void *conexion_memoria(void* socket_memoria);
t_pcb* crear_estructura_pcb(t_list* lista_instrucciones, t_list* tabla_segmentos, uint32_t socket_consola);

// Funciones de planificacion ===============================

//Inicio de planificacion y planificadores
void iniciar_planificacion();
void* planificador_largo_plazo(void*);
void* planificador_corto_plazo(void*);

void inicializar_mutex();
void inicializar_semaforos_sincronizacion(uint32_t grado_multiprogramacion);

void agregar_pcb_a_cola(t_pcb*,pthread_mutex_t, t_queue*);
t_pcb* quitar_pcb_de_cola(pthread_mutex_t, t_queue* cola);

void agregar_pcb_a_lista(t_pcb*,pthread_mutex_t, t_list*);
t_pcb* quitar_primer_pcb_de_lista(pthread_mutex_t, t_list* cola);
int algoritmo_planificacion_tiene_desalojo();

//Funciones del planificador de largo plazo =================

//Espera para poder agregar un pcb de new a ready
void pcb_a_dispatcher();

//Espera para finalizar un pcb
void* finalizador_procesos(void*);

//Funciones del planificador de corto plazo =================

void* manejador_estado_ready(void*);
void* manejador_estado_running(void*);
void* manejador_estado_blocked_pf(void*);
void* manejador_estado_blocked_screen(void* x);
void* manejador_estado_blocked_keyboard(void* x);
void* clock_interrupt(void* socket);

void* bloquear_pcb(void* indice);
t_pcb* buscar_pcb_a_bloquear(dispositivo io);
t_pcb* obtener_pcb_de_lista_segun_socket(int socket,t_list* lista_pcb,pthread_mutex_t mutex);
int obtener_indice_pcb_segun_socket(int socket,t_list* lista_pcb,pthread_mutex_t mutex);
void desbloquear_pcb_screen(int socket);
void desbloquear_pcb_keyboard(int socket,int input);

dispositivo obtener_dispositivo(t_pcb* pcb);
uint32_t obtener_tiempo_bloqueo(t_pcb* pcb);

void agregar_a_ready(t_pcb* pcb, op_code motivo,estado_pcb anterior);
t_pcb* quitar_de_ready(estado_pcb* cola_ready);
int algoritmo_es_feedback();
void logear_cambio_estado(t_pcb* pcb,estado_pcb anterior, estado_pcb actual);
char* traducir_estado_pcb(estado_pcb);

registro_cpu* obtener_registro_por_bloqueo_pantalla_teclado(t_pcb* pcb);
t_instruccion* obtener_instruccion_anterior(t_pcb* pcb);
void actualizar_registro_por_teclado(t_pcb* pcb, uint32_t input);

void crear_estructuras_memoria(t_pcb* pcb);

#endif //TP_2022_2C_CHAMACOS_KERNEL_H