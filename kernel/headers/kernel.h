#ifndef TP_2022_2C_CHAMACOS_KERNEL_H
#define TP_2022_2C_CHAMACOS_KERNEL_H

#include "../../shared/headers/shared.h"

#define LOG_FILE "kernel.log"
#define LOG_NAME "kernel_log"

t_config* consola_config;
t_config* communication_config;
t_log* logger;

pthread_t thread_escucha_memoria;
pthread_t thread_escucha_dispatch;
pthread_t thread_escucha_interrupt;

//Obtiene el ip y puerto del kernel para iniciar el servidor, devuele el socket
int levantar_servidor();

void esperar_modulos(int socket_srv);
void esperar_consolas(int socket_srv);

void *conexion_dispatch(void* socket_dispatch);
void *conexion_interrupt(void* socket_interrupt);
void *conexion_memoria(void* socket_memoria);

#endif //TP_2022_2C_CHAMACOS_KERNEL_H
