#include "shared_utils.h"

/*
 *
 * Funciones del Logger-Config
 *  
 */

t_log* iniciar_logger(char* file, char* log_name) {
    t_log* nuevo_logger = log_create(file, log_name, 1, LOG_LEVEL_DEBUG );
    return nuevo_logger;
}

t_config* iniciar_config(char* file) {
    t_config* nuevo_config;
    if((nuevo_config = config_create(file)) == NULL) {
        printf(RED"No se pudo leer la configuracion\n");
        exit(EXIT_FAILURE);
    }
    return nuevo_config;
}

t_config* init_connection_config() {
    return iniciar_config(CONNECTION_FILE);
}

void logear_instruccion(t_log* logger, t_instruccion* instruccion){
    char* instruccion_traducida = traducir_instruccion_string(instruccion);
    log_debug(logger,string_from_format("INSTRUCCION: %s",instruccion_traducida));
}

char* traducir_instruccion_string(t_instruccion* instr){
    instr_code cop = instr->codigo_operacion;
    operando op1 = instr->parametros[0];
    operando op2 = instr->parametros[1];
    switch(cop){
        case SET:       return string_from_format("%-8s %-10s %5d","SET",traducir_registro_cpu(op1),op2);
	    case ADD:       return string_from_format("%-8s %-10s %5s","ADD",traducir_registro_cpu(op1),traducir_registro_cpu(op2));
	    case MOV_IN:    return string_from_format("%-8s %-10s %5d","MOV_IN",traducir_registro_cpu(op1),op2);
	    case MOV_OUT:   return string_from_format("%-8s %-10d %5s","MOV_OUT",op1,traducir_registro_cpu(op2));
	    case IO:        return string_from_format("%-8s %-10s %5d","I/O",traducir_dispositivo(op1),op2);
	    case EXIT:      return string_from_format("%-8s %-10d %5d","EXIT",op1,op2);
        default: return string_from_format("ERROR - COP INVALIDO: %d",cop);
    }
}

char* traducir_registro_cpu(registro_cpu registro){
    switch(registro){
        case AX: return "AX";
        case BX: return "BX";
        case CX: return "CX";
        case DX: return "DX";
        default: return "ERROR - REGISTRO INVALIDO";
    }
}

char* traducir_dispositivo(dispositivo disp){
    if(disp == DISCO)           return "DISCO";
    else if( disp == IMPRESORA) return "IMPRESORA";
    else return "ERROR - DISPOSITIVO INVALIDO";
}

/*
 *
 * Funciones de Comunicacion
 *  
 */

int crear_conexion(char* ip, int puerto){
    int socket_cliente = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_cliente == -1) {
        perror("Hubo un error al crear el socket del servidor");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in direccionServer;
    direccionServer.sin_family = AF_INET;
    direccionServer.sin_addr.s_addr = inet_addr(ip);
    direccionServer.sin_port = htons(puerto);
    memset(&(direccionServer.sin_zero), '\0', 8); //se rellena con ceros para que tenga el mismo tamaño que socketaddr

    verificar_connect(socket_cliente, &direccionServer);

    return socket_cliente;
}

void verificar_connect(int socket_cliente, struct sockaddr_in *direccion_server) {
    if (connect(socket_cliente, (void*) direccion_server, sizeof((*direccion_server))) == -1) {
        perror("Hubo un problema conectando al servidor");
        close(socket_cliente);
        exit(EXIT_FAILURE);
    }
}

