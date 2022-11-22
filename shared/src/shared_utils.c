#include "../headers/shared.h"

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
        printf(RED"No se pudo leer la configuracion: %s\n",file);
        exit(EXIT_FAILURE);
    }
    return nuevo_config;
}

void logear_instruccion(t_log* logger, t_instruccion* instruccion){
    char* instruccion_traducida = traducir_instruccion_string(instruccion);
    log_info(logger,string_from_format(GRN"INSTRUCCION: %s"WHT,instruccion_traducida));
}

registro_cpu* obtener_registro(t_pcb* pcb, registro_cpu registro){
    switch (registro){
        case AX: return &(pcb->registros_pcb->registro_ax);
        case BX: return &(pcb->registros_pcb->registro_bx);
        case CX: return &(pcb->registros_pcb->registro_ax);
        case DX: return &(pcb->registros_pcb->registro_ax);
        default: 
            printf(RED"Error al obtener el puntero del registro\n"WHT);
            exit(EXIT_FAILURE);
    }
}

void enviar_entero(int cliente_fd, uint32_t valor, op_code opCode) {
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = opCode;
    agregar_entero(paquete, valor);
    enviar_paquete(paquete, cliente_fd);
    eliminar_paquete(paquete);
}