#include "../headers/consola.h"
#include <stdint.h>

int main(int argc, char* argv[]) {

    validar_argumentos_main(argc);

    char* archivo = argv[1];
    char* CONFIG_FILE = argv[2];

    logger = iniciar_logger(LOG_FILE, LOG_NAME);
    consola_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();

    FILE* archivo_pseudocodigo = fopen(archivo,"r");
	t_list* instrucciones = parsear_instrucciones(logger, archivo_pseudocodigo);

    char* IP_KERNEL = config_get_string_value(communication_config,"IP_KERNEL");
    int PUERTO_KERNEL = config_get_int_value(communication_config,"PUERTO_KERNEL");

    int socket_kernel = crear_conexion(IP_KERNEL, PUERTO_KERNEL);
    enviar_handshake_inicial(socket_kernel, CONSOLA, logger);

    char** segmentos_config = config_get_array_value(consola_config,"SEGMENTOS");
    t_list* segmentos = convertir_segmentos(segmentos_config); 

	int tiempo_respuesta = config_get_int_value(consola_config,"TIEMPO_DE_RESPUESTA");
    log_info(logger,string_from_format("Retardo de impresion: %ds",tiempo_respuesta/1000));

    enviar_lista_instrucciones_segmentos(socket_kernel, instrucciones, segmentos);
    log_info(logger,"Envie lista de instrucciones y segmentos");
    list_destroy(instrucciones);
    list_destroy(segmentos);


    while(socket_kernel!=-1){

        printf("\n");
        log_info(logger, "Esperando instrucciones.");
        op_code cod_op = recibir_operacion(socket_kernel);
        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(socket_kernel, logger);
                break;
            case IMPRIMIR_VALOR:
				log_info(logger, BLU"Kernel envió un valor para imprimir"WHT);
				uint32_t valor = recibir_valor(socket_kernel);
				log_info(logger, string_from_format(CYN"Valor recibido: %d"WHT, valor));
				usleep(tiempo_respuesta*1000);
                enviar_codigo_op(socket_kernel,IMPRIMIR_VALOR);
				break;
			case INPUT_VALOR:
				log_info(logger, BLU"Kernel solicitó un valor"WHT);
				printf("Ingrese un valor: ");
				uint32_t input;
				scanf("%d", &input);
                log_info(logger,BLU"Valor enviado al kernel."WHT);
				enviar_input_valor(input, socket_kernel);
				break;
            case FINALIZAR_PROCESO:
                terminar_programa(socket_kernel,logger,communication_config);
                return EXIT_SUCCESS;
			default:
                log_trace(logger, "Operación desconocida en consola");
                terminar_programa(socket_kernel, logger, communication_config);
                break;
        }
    }
    return EXIT_SUCCESS;

}


void validar_argumentos_main(int argumentos){
    uint32_t expected = 3;
    if (argumentos < expected){
		printf(RED"");
		printf("Cantidad de parametros incorrectos.\n");
        printf(" > Expected: %d - Given: %d\n",expected,argumentos);
		printf("   1- Archivo de pseudocodigo.\n   2- Archivo de configuracion\n");
		printf(RESET"");
		exit(argumentos);
	}
}

t_list* parsear_instrucciones(t_log* logger, FILE* archivo) {

	t_list* lista_instrucciones = list_create();

    char* registro = NULL;
    size_t tam_registro=0;

    while(getline(&registro, &tam_registro, archivo) != -1){
        t_instruccion* instr = generar_instruccion(registro);
        logear_instruccion(logger, instr);
        list_add(lista_instrucciones, instr);
    }

    return lista_instrucciones;
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
            if(string_equals_ignore_case(operando1,"PANTALLA") || string_equals_ignore_case(operando1,"TECLADO")){
                instruccion->parametros[1]=obtener_registro_cpu(operando2);
            }else{
                instruccion->parametros[1]=atoi(operando2);
            }
            break;
        case EXIT: default:
            instruccion->parametros[0]=0;
            instruccion->parametros[1]=0;
            break;
    }
    return instruccion;
}

instr_code obtener_cop(char* operacion){
	if (string_contains(operacion,"SET")) 		    return SET;
	else if (string_contains(operacion,"ADD")) 	    return ADD;
	else if (string_contains(operacion,"MOV_IN")) 	return MOV_IN;
	else if (string_contains(operacion,"MOV_OUT")) 	return MOV_OUT;
	else if (string_contains(operacion,"I/O"))      return IO;
	else                                            return EXIT;
}

registro_cpu obtener_registro_cpu(char* registro){
    if(string_contains(registro,"AX"))      return AX;
    else if(string_contains(registro,"BX")) return BX;
    else if(string_contains(registro,"CX")) return CX;
    else                                    return DX;
}

dispositivo obtener_dispositivo(char* dispositivo){
    if (string_contains(dispositivo,"DISCO"))           return DISCO;
    else if(string_contains(dispositivo,"IMPRESORA"))   return IMPRESORA;
    else if(string_contains(dispositivo,"PANTALLA"))    return PANTALLA;
    else                                                return TECLADO;
}

t_list* convertir_segmentos(char** segmentos_config){
    t_list* lista_segmentos = list_create();
    uint32_t tam = string_array_size(segmentos_config);
    for(int i=0; i < tam; i++){
        void* valor_segmento= malloc(sizeof(uint32_t));
        int valor = atoi(segmentos_config[i]);
        memcpy(valor_segmento,&valor,sizeof(uint32_t));
        list_add(lista_segmentos, valor_segmento);
    }
    return lista_segmentos;

}

void terminar_programa(uint32_t conexion, t_log* logger, t_config* config) {

    log_info(logger, "Consola: Terminando programa...");
    log_destroy(logger);
    if(config!=NULL) {
        config_destroy(config);
    }
    close(conexion);
}
