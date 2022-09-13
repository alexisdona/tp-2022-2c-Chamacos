#include "consola.h"

int main(int argc, char* argv[]) {

    if(argc < 3){
		printf(RED"");
		printf("Cantidad de parametros incorrectos.\n");
		printf("1- Archivo de pseudocodigo.\n2- Archivo de configuracion\n");
		printf(RESET"");
		return argc;
	}

    char* archivo = argv[1];
    char* config_file = argv[2];

    inicializar_logger_configs(config_file);
    FILE* archivo_pseudocodigo = fopen(archivo,"r");
	t_queue* instrucciones = parsear_instrucciones(logger, archivo_pseudocodigo);

    char* ip = config_get_string_value(communication_config,"IP_KERNEL");
    int puerto = config_get_int_value(communication_config,"PUERTO_KERNEL");


	return EXIT_SUCCESS;
}

void inicializar_logger_configs(char* config_file){
    logger = iniciar_logger(LOG_FILE, LOG_NAME);
    consola_config = iniciar_config(config_file);
    communication_config = init_connection_config();
}


t_queue* parsear_instrucciones(t_log* logger, FILE* archivo) {

	t_queue* pseudocodigo = queue_create();

    char* registro = NULL;
    size_t tam_registro=0;

    while(getline(&registro, &tam_registro, archivo) != -1){
        t_instruccion* instr = generar_instruccion(registro);
        logear_instruccion(logger,instr);
        queue_push(pseudocodigo, instr);
    }

    return pseudocodigo;
}

t_instruccion* generar_instruccion(char* registro){
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));

    char* operacion = strtok_r(registro," ",&registro);
    char* operando1 = strtok_r(NULL," ",&registro);
    char* operando2 = strtok_r(NULL,"\n",&registro);

    instruccion->codigo_operacion = obtener_cop(operacion);
    switch(instruccion->codigo_operacion){
        case SET: case MOV_IN:
            instruccion->parametros[0]=obtener_registro_cpu(operando1);
    		instruccion->parametros[1]=atoi(operando2);
            break;
        case ADD:
            instruccion->parametros[0]=obtener_registro_cpu(operando1);
            instruccion->parametros[1]=obtener_registro_cpu(operando2);
            break;
        case MOV_OUT:
            instruccion->parametros[0]=atoi(operando1);
            instruccion->parametros[1]=obtener_registro_cpu(operando2);
            break;
        case IO:
            instruccion->parametros[0]=obtener_dispositivo(operando1);
            instruccion->parametros[1]=atoi(operando2);
            break;
        case EXIT: default:
            instruccion->parametros[0]=0;
            instruccion->parametros[1]=0;
            break;
    }
    return instruccion;
}

instr_code obtener_cop(char* operacion){
	if(string_contains(operacion,"SET")) 		    return SET;
	else if(string_contains(operacion,"ADD")) 	    return ADD;
	else if(string_contains(operacion,"MOV_IN")) 	return MOV_IN;
	else if(string_contains(operacion,"MOV_OUT")) 	return MOV_OUT;
	else if(string_contains(operacion,"I/O"))       return IO;
	else                                            return EXIT;
}

registro_cpu obtener_registro_cpu(char* registro){
    if(string_contains(registro,"AX"))      return AX;
    else if(string_contains(registro,"BX")) return BX;
    else if(string_contains(registro,"CX")) return CX;
    else                                    return DX;
}

dispositivo obtener_dispositivo(char* dispositivo){
    if(string_contains(dispositivo,"DISCO")) return DISCO;
    else                                     return IMPRESORA;
}