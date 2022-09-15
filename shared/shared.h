#ifndef TP_2022_1C_ECLIPSO_SHARED_H
#define TP_2022_1C_ECLIPSO_SHARED_H

#define _GNU_SOURCE

//Includes
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdint.h>
#include<netdb.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<commons/string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>

//Para prints de colores
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define CONSOLA         0
#define KERNEL          1
#define CPU_DISPATCH    2
#define CPU_INTERRUPT   3
#define MEMORIA         4


#define CONNECTION_FILE "../../connection.config"
extern char* LOG_FILE;
extern char* LOG_NAME;

typedef uint32_t operando;
typedef enum
{
    SET,
    ADD,
    MOV_IN,
    MOV_OUT,
    IO,
    EXIT
} instr_code;

typedef enum{
    AX,
    BX,
    CX,
    DX
} registro_cpu;

typedef enum{
    DISCO,
    IMPRESORA
} dispositivo;

typedef struct{
    instr_code codigo_operacion;
    operando parametros[2];
} t_instruccion;

//Funciones comunes a los modulos ===========================

//Valida la cantidad de argumentos para iniciar el modulo
//1 implementacion por modulo
void validar_argumentos_main(int cantidad_argumentos);

//Logger-Config==============================================

//Devuelve un logger nuevo con el archivo de logs y su nombre
t_log* iniciar_logger(char* file, char* log_name);
//Devuelve un config nuevo con el archivo de config
t_config* iniciar_config(char* file);
//Setea la configuracion del archivo de comunicacion
t_config* init_connection_config();

//Conjunto de traductores para obtener las instrucciones 
//de manera tal que sea legible para ser logeadas
void logear_instruccion(t_log* logger, t_instruccion* instrucicon);
char* traducir_instruccion_string(t_instruccion* instruccion);
char* traducir_registro_cpu(registro_cpu registro);
char* traducir_dispositivo(dispositivo disp);

//Comunicacion===============================================

//Conexion cliente-------------------------------------------
//Genera un socket para un cliente y verifica la conexion
int crear_conexion(char* ip, int puerto);
void verificar_connect(int socket_cliente, struct sockaddr_in *direccion_server);

//Conexion servidor------------------------------------------
//Genera el socket del servidor y realiza el bind y listen
int iniciar_servidor(char* ip, char* puerto, t_log* logger);
void verificar_bind(int socket_kernel,  struct addrinfo *kernelinfo);
void verificar_listen(int socket);
//Se bloquea esperando por un nuevo cliente
int esperar_cliente(int socket_server, t_log* logger);

//Valida que el identificador de un modulo sea admisible
int modulo_valido(uint32_t modulo);
//Retorna el nombre del modulo o socket-modulo dado un identificador
char* identificadores_modulo(uint32_t id_modulo);
//Envia por el socket el identificador de su modulo y recibe el modulo al que se quiere conectar
/*Ejemplo: al conectarse la consola con el kernel, obtiene el codigo del kernel
    enviar_handshake_inicial(sc-consola,CONSOLA,logger) -> KERNEL
*/
int enviar_handshake_inicial(int socket, uint32_t su_codigo, t_log* logger);
//Recibe por el socket el identificador del modulo que se conecta y envia el codigo de si mismo
/*Ejemplo: al recibir la conexion de la consola, recibe el codigo CONSOLA y envia KERNEL
    recibir_handshake_inicial(sc-kernel,KERNEL,logger) -> CONSOLA
*/
int recibir_handshake_inicial(int socket, uint32_t mi_codigo, t_log* logger);


#endif //TP_2022_1C_ECLIPSO_SHARED_H