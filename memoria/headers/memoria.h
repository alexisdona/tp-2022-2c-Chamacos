#ifndef TP_2022_2C_CHAMACOS_MEMORIA_H
#define TP_2022_2C_CHAMACOS_MEMORIA_H

#include "../../shared/headers/shared.h"
#include <commons/bitarray.h>

#define LOG_FILE "memoria.log"
#define LOG_NAME "memoria_log"
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

typedef struct {
    uint32_t numero_pagina;
    uint32_t pid;
    uint32_t frame;
    uint modificado;
    uint uso;
    uint presencia;
    uint32_t posicion_swap;
} t_registro_tabla_paginas;

typedef struct {
    t_list* registro_tabla_paginas;
} t_tabla_paginas;

typedef struct {
    uint32_t numero_pagina;
    uint32_t frame;
    uint uso;
    uint modificado;
} t_frame_ocupado;

t_queue* paginas_frames;
t_dictionary* frames_ocupados;
t_config* memoria_config;
t_config* communication_config;
t_log* logger;
void* espacio_usuario_memoria;
char** lista_dispositivos;
char* CONFIG_FILE;
int socket_srv_memoria;
uint32_t tamanio_memoria;
uint32_t tamanio_pagina;
uint32_t  entradas_por_tabla;
uint32_t retardo_memoria;
uint32_t tamanio_swap;
uint32_t marcos_por_proceso;
void* espacio_usuario_memoria;
t_bitarray* frames_disponibles;
void* bloque_frames_libres;
char* algoritmo_reemplazo;
char* path_swap;
char* ip_kernel;
int puerto_kernel;
char* ip_cpu;
int puerto_cpu;
int socket_kernel;
int socket_cpu;
uint32_t puntero_swap = 0;

t_list* lista_pid_frame;
t_queue* cola_pid_registro_tabla_paginas;

pthread_mutex_t mutex_listas_frames_pendientes;
pthread_mutex_t mutex_puntero_swap;

pthread_t thread_escucha_kernel;
pthread_t thread_escucha_cpu;
pthread_t buscador_marcos;

t_list* registros_tabla_paginas;
t_list* tabla_paginas;


sem_t buscar_frame;

pthread_mutex_t mutex_tabla_paginas;

//Obtiene el ip y puerto del kernel para iniciar el servidor, devuele el socket
int levantar_servidor();
void iniciar_estructuras_administrativas_kernel();
void crear_espacio_usuario();
uint32_t obtener_ultima_posicion_swap();
void crear_archivo_swap();
void mostrar_contenido_archivo_swap();
void levantar_config();
void procesar_conexion(void* void_args);
void escuchar_cliente(int socket_server, t_log* logger);
void *conexion_kernel(void* socket);
void *conexion_cpu(void* socket);
void handshake_cpu_memoria(int socket_destino, uint32_t tamanio_pagina, uint32_t cantidad_entradas_tabla);
void actualizar_puntero_swap();
int obtener_numero_frame_libre();
uint32_t crear_estructuras_administrativas_proceso(uint32_t tamanio_segmento, uint32_t pid, uint32_t contador_paginas);
void* obtener_bloque_proceso_desde_swap(uint32_t posicion_swap);
uint32_t obtener_cantidad_marcos_ocupados_proceso(uint32_t id_proceso_marco);
//void enviar_page_fault_cpu(int cliente_fd, op_code cod_op, int marco);
void buscar_frame_libre_proceso(t_registro_tabla_paginas* registro_tabla_paginas);
void enviar_marco(int cliente_fd, int marco);

void* buscar_marcos_para_procesos(void*);
void actualizar_lista_frames_pendientes(t_registro_tabla_paginas* registro);
t_registro_tabla_paginas *obtener_tupla_elementos_lista_frames_pendientes();
void agregar_a_cola_frames_por_paginas(t_registro_tabla_paginas* registro_tabla_paginas);
void actualizar_bit_modificado(t_registro_tabla_paginas* registro);
void actualizar_bit_uso(t_registro_tabla_paginas* registro);
t_registro_tabla_paginas *obtener_registro_tabla_paginas(uint32_t indice_tabla_paginas, uint32_t numero_pagina);
void ejecutar_clock(t_registro_tabla_paginas * registro_tabla_paginas_nuevo);
void ejecutar_clock_modificado(t_registro_tabla_paginas * registro_tabla_paginas_nuevo);


#endif //TP_2022_2C_CHAMACOS_MEMORIA_H
