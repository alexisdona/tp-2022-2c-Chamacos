#include "../headers/kernel.h"

int main(int argc, char* argv[]) {

    validar_argumentos_main(argc);

    char* CONFIG_FILE = argv[1];

    logger = iniciar_logger(LOG_FILE,LOG_NAME);
    consola_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();

    int socket_srv_kernel = levantar_servidor();
    esperar_modulos(socket_srv_kernel);
    esperar_consolas(socket_srv_kernel);

}

void validar_argumentos_main(int argumentos){
    uint32_t expected = 2;
    if(argumentos < expected){
		printf(RED"");
		printf("Cantidad de parametros incorrectos.\n");
        printf(" > Expected: %d - Given: %d\n",expected,argumentos);
		printf("   1- Archivo de configuracion\n");
		printf(RESET"");
		exit(argumentos);
	}
}

int levantar_servidor(){
    char* ip_kernel = config_get_string_value(communication_config,"IP_KERNEL");
    char* puerto_kernel = config_get_string_value(communication_config,"PUERTO_KERNEL");

    return iniciar_servidor(ip_kernel,puerto_kernel,logger);
}

void esperar_consolas(int socket_srv){
    log_info(logger,"Esperando consolas..");
    int socket_consola = esperar_cliente(socket_srv,logger);
    uint32_t modulo = recibir_handshake_inicial(socket_consola,KERNEL,logger);

    while(socket_consola > -1){
        op_code codigo_operacion = recibir_operacion(socket_consola);
        switch(codigo_operacion){
            case LISTA_INSTRUCCIONES:
                log_info(logger,"Recibiendo una lista de instrucciones");
                t_list* instrucciones = recibir_lista_instrucciones(socket_consola);
                uint32_t* segmentos = recibir_segmentos(socket_consola);

                printf("Instrucciones:\n");
                for(int i=0; i<list_size(instrucciones); i++){
                    logear_instruccion(logger,(t_instruccion*) list_get(instrucciones,i));
                }
                printf("Segmentos:\n");
                for(int i=0; i < 4; i++){
                    printf("Segmento: %d\n",segmentos[i]);
                }
                break;
            default: ;
        }
    }
}

void esperar_modulos(int socket_srv){
    
    uint32_t cantidad_modulos = 3; //Dispatch - Interrupt - Memoria

    for(int i=0; i < cantidad_modulos; i++){

        int socket_cliente = esperar_cliente(socket_srv,logger);
        uint32_t modulo = recibir_handshake_inicial(socket_cliente,KERNEL,logger);
        
        switch(modulo){
            case CPU_DISPATCH:
                        pthread_create(&thread_escucha_dispatch, NULL, conexion_dispatch, (void*) (intptr_t)socket_cliente);
                        break;
            case CPU_INTERRUPT:
                        pthread_create(&thread_escucha_interrupt, NULL, conexion_interrupt, (void*) (intptr_t)socket_cliente);
                        break;
            case MEMORIA:
                        pthread_create(&thread_escucha_memoria, NULL, conexion_memoria, (void*) (intptr_t)socket_cliente);
                        break;
            default: 
                        log_error(logger,string_from_format("Modulo desconocido actualmente %d",modulo));
                        i--;
        }
    }    
}

void *conexion_dispatch(void* socket){
    int socket_dispatch = (intptr_t) socket;
    while(socket_dispatch != -1){
        uint32_t op = recibir_operacion(socket_dispatch);
    }
}

void *conexion_interrupt(void* socket){
    int socket_interrupt = (intptr_t) socket;
    while(socket_interrupt != -1){
        uint32_t op = recibir_operacion(socket_interrupt);
    }
}

void *conexion_memoria(void* socket){
    int socket_memoria = (intptr_t) socket;
    while(socket_memoria != -1){
        uint32_t op = recibir_operacion(socket_memoria);
    }
}