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
#include<pthread.h>
#include<semaphore.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<commons/string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include "pcb.h"

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

#define PANTALLA        100
#define TECLADO         200

#define CONNECTION_FILE "../../connection.config"

extern char* LOG_FILE;
extern char* LOG_NAME;
extern char** lista_dispositivos;

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

/*
typedef enum{
    DISCO,
    IMPRESORA,
    PANTALLA,
    TECLADO
} dispositivo;
*/

typedef uint32_t dispositivo;

typedef struct{
    instr_code codigo_operacion;
    operando parametros[2];
} t_instruccion;


typedef enum {
    MENSAJE,
	IMPRIMIR_VALOR,
	INPUT_VALOR,
    LISTA_INSTRUCCIONES_SEGMENTOS,
    PCB,
    INTERRUPCION,
    CONTINUA_PROCESO,
    BLOQUEAR_PROCESO_IO,
    BLOQUEAR_PROCESO_PANTALLA,
    BLOQUEAR_PROCESO_TECLADO,
    PAGE_FAULT,
    DESALOJAR_PROCESO,
    FINALIZAR_PROCESO,
    ACTUALIZAR_INDICE_TABLA_PAGINAS,
    CREAR_ESTRUCTURAS_ADMIN,
    ESCRIBIR_MEMORIA,
    LEER_MEMORIA,
    OBTENER_MARCO,
    TERMINAR_PROCESO,
    HANDSHAKE_CPU_MEMORIA,
    SEGMENTATION_FAULT,
    PAGE_FAULT_ATENDIDO
} op_code;

typedef struct{
    size_t size;
    void* stream;
} t_buffer;

typedef struct{
    op_code codigo_operacion;
    t_buffer* buffer;
} t_paquete;

typedef struct {
    t_log* log;
    int fd;
    char* nombre;
} t_procesar_conexion_attrs;


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
registro_cpu* obtener_registro(t_pcb* pcb, registro_cpu registro);

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

//Valida que el identificador de un modulo sea admisible
int modulo_valido(uint32_t modulo);

//Retorna el nombre del modulo o socket-modulo dado un identificador
char* identificadores_modulo(uint32_t id_modulo);

//Crea y retorna un t_paquete y le asigna memoria
t_paquete* crear_paquete();

//Crea un buffer para un paquete;
void crear_buffer(t_paquete* paquete);

//Agrega un entero a un paquete
void agregar_entero(t_paquete * paquete, uint32_t entero);

//Obtiene las instrucciones deserialiada
t_list *deserializar_lista_instrucciones(void *stream, uint32_t tamanio_lista_instrucciones);

//Obtiene los segmentos deserialiados
t_list *deserializar_lista_segmentos(void *stream, uint32_t tamanio_tabla_segmentos);


//Serializa el paquete para poder ser enviado
void* serializar_paquete(t_paquete* paquete, size_t bytes);

//Envia el paquete por el socket
int enviar_paquete(t_paquete* paquete, int socket_destino);

//Destruye el paquete y su memoria
void eliminar_paquete(t_paquete* paquete);

//Obtiene el codigo de operacion un paquete a recibir
op_code recibir_operacion(int socket_cliente);

//Obtiene el resto de la informaci{on de un paquete
void* recibir_buffer(int socket_cliente);

void enviar_mensaje(char* mensaje, int socket);
void recibir_mensaje(int socket_cliente, t_log *logger);

t_list* recibir_lista_instrucciones(int socket);
t_list* recibir_lista_segmentos(int socket);

void agregar_lista_instrucciones( t_paquete *paquete, t_list *instrucciones);

void enviar_page_fault_cpu(int socket, uint32_t marco);
void enviar_numero(int socket, op_code codigo_op, uint32_t numero);
void enviar_imprimir_valor(uint32_t numero, int socket);
uint32_t recibir_valor(int socket);
uint32_t deserializar_entero(void* stream);
void enviar_esperar_input_valor(int socket);
void enviar_input_valor(uint32_t valor, int socket);
void enviar_codigo_op(int socket, op_code codigo);

//Agrega una instruccion al paquete
void agregar_instruccion(t_paquete* paquete, void* instruccion);
void enviar_PCB(int socket_destino, t_pcb* pcb, op_code codigo_operacion);
t_pcb* recibir_PCB(int socket_desde);
void enviar_interrupcion(int socket);
void enviar_entero(int cliente_fd, uint32_t valor, op_code opCode);
void enviar_entero8bytes(int cliente_fd, int valor, op_code opCode);
void agregar_entero8bytes(t_paquete* paquete, int entero);


#endif //TP_2022_1C_ECLIPSO_SHARED_H