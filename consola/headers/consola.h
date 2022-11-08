#ifndef TP_2022_2C_CHAMACOS_CONSOLA_H
#define TP_2022_2C_CHAMACOS_CONSOLA_H


#include "../../shared/headers/shared.h"

#define LOG_FILE "consola.log"
#define LOG_NAME "consola_log"

t_config* consola_config;
t_config* communication_config;
t_log* logger;
char** lista_dispositivos;

t_list* parsear_instrucciones(t_log* logger, FILE* archivo);

t_instruccion* generar_instruccion(char* registro);

instr_code obtener_cop(char* operacion);

registro_cpu obtener_registro_cpu(char* registro);

dispositivo obtener_dispositivo(char* dispositivo);

t_list* convertir_segmentos(char** segmentos_config);

void enviar_lista_instrucciones_segmentos(uint32_t socket_destino, t_list* instrucciones, t_list* tabla_segmentos);

void terminar_programa(uint32_t conexion, t_log* logger, t_config* config);


#endif //TP_2022_2C_CHAMACOS_CONSOLA_H
