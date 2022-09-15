#ifndef TP_2022_2C_CHAMACOS_CONSOLA_H
#define TP_2022_2C_CHAMACOS_CONSOLA_H

#include "../../shared/shared.h"

#define LOG_FILE "consola.log"
#define LOG_NAME "consola_log"

t_config* consola_config;
t_config* communication_config;
t_log* logger;

t_queue* parsear_instrucciones(t_log* logger, FILE* archivo);
t_instruccion* generar_instruccion(char* registro);
instr_code obtener_cop(char* operacion);
registro_cpu obtener_registro_cpu(char* registro);
dispositivo obtener_dispositivo(char* dispositivo);

#endif //TP_2022_2C_CHAMACOS_CONSOLA_H