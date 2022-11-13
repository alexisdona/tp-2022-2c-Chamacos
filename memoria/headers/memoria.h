#ifndef TP_2022_2C_CHAMACOS_MEMORIA_H
#define TP_2022_2C_CHAMACOS_MEMORIA_H

#include "../../shared/headers/shared.h"
#include <commons/bitarray.h>

#define LOG_FILE "memoria.log"
#define LOG_NAME "memoria_log"

typedef struct {
    uint32_t numero_pagina;
    uint32_t frame;
    uint modificado;
    uint uso;
    uint presencia;
    uint32_t posicion_swap;
} t_registro_tabla_paginas;

typedef struct {
    t_list* registro_tabla_paginas;
} t_tabla_paginas;

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
void* espacio_usuario_memoria;
t_bitarray* frames_disponibles;
void* bloque_frames_lilbres;
char* algoritmo_reemplazo;
char* path_swap;
char* ip_kernel;
int puerto_kernel;
char* ip_cpu;
int puerto_cpu;
int socket_kernel;
int socket_cpu;

pthread_t thread_escucha_kernel;
pthread_t thread_escucha_cpu;

t_list* registros_tabla_paginas;
t_list* tabla_paginas;

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
#endif //TP_2022_2C_CHAMACOS_MEMORIA_H
