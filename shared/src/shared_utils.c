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

char* traducir_instruccion_string(t_instruccion* instr){
    instr_code cop = instr->codigo_operacion;
    operando op1 = instr->parametros[0];
    operando op2 = instr->parametros[1];
    switch(cop){
        case SET:       return string_from_format("%-8s %-10s %5d","SET",traducir_registro_cpu(op1),op2);
	    case ADD:       return string_from_format("%-8s %-10s %5s","ADD",traducir_registro_cpu(op1),traducir_registro_cpu(op2));
	    case MOV_IN:    return string_from_format("%-8s %-10s %5d","MOV_IN",traducir_registro_cpu(op1),op2);
	    case MOV_OUT:   return string_from_format("%-8s %-10d %5s","MOV_OUT",op1,traducir_registro_cpu(op2));
	    case IO:        if(op1==PANTALLA || op1==TECLADO) return string_from_format("%-8s %-10s %5s","I/O",traducir_dispositivo(op1),traducir_registro_cpu(op2));
                        else return string_from_format("%-8s %-10s %5d","I/O",traducir_dispositivo(op1),op2);
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
    switch(disp){
        case DISCO:     return "DISCO";
        case IMPRESORA: return "IMPRESORA";
        case PANTALLA:  return "PANTALLA";
        case TECLADO:   return "TECLADO";
        default: return "ERROR - DISPOSITIVO INVALIDO";
    }
}

registro_cpu* obtener_registro(t_pcb* pcb, registro_cpu registro){
    switch (registro){
        case AX: return &(pcb->registros_pcb.registro_ax);
        case BX: return &(pcb->registros_pcb.registro_bx);
        case CX: return &(pcb->registros_pcb.registro_ax);
        case DX: return &(pcb->registros_pcb.registro_ax);
        default: 
            printf(RED"Error al obtener el puntero del registro\n"WHT);
            exit(EXIT_FAILURE);
    }
}