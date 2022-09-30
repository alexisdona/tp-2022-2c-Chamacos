#ifndef TP_2022_2C_CHAMACOS_KERNEL_H
#define TP_2022_2C_CHAMACOS_KERNEL_H

#include "../../shared/headers/shared.h"
#include "pthread.h"

#define LOG_FILE "kernel.log"
#define LOG_NAME "kernel_log"

t_config* consola_config;
t_config* communication_config;
t_log* logger;

//Obtiene el ip y puerto del kernel para iniciar el servidor, devuele el socket
int levantar_servidor();

#endif //TP_2022_2C_CHAMACOS_KERNEL_H
