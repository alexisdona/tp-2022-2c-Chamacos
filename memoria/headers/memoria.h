#ifndef TP_2022_2C_CHAMACOS_MEMORIA_H
#define TP_2022_2C_CHAMACOS_MEMORIA_H

#include "../../shared/headers/shared.h"

#define LOG_FILE "memoria.log"
#define LOG_NAME "memoria_log"

t_config* memoria_config;
t_config* communication_config;
t_log* logger;

//Obtiene el ip y puerto del kernel para iniciar el servidor, devuele el socket
int levantar_servidor();

#endif //TP_2022_2C_CHAMACOS_MEMORIA_H
