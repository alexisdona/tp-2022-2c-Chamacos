#include "../headers/kernel.h"

int main(int argc, char* argv[]) {

    validar_argumentos_main(argc);

    char* CONFIG_FILE = argv[1];

    logger = iniciar_logger(LOG_FILE, LOG_NAME);
    consola_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();

    int socket_srv_kernel = levantar_servidor();
    int socket_cliente = esperar_cliente(socket_srv_kernel,logger);
    log_info(logger, "### ESPERANDO CONSOLAS ###");

    while(socket_cliente > -1){
        op_code codigo_operacion = recibir_operacion(socket_cliente);
        switch(codigo_operacion){
            case MENSAJE:
                recibir_mensaje(socket_cliente, logger);
                break;
            case LISTA_INSTRUCCIONES:
                log_info(logger,"Recibiendo una lista de instrucciones");
                t_list* instrucciones = recibir_lista_instrucciones(socket_cliente);
                printf("Instrucciones:\n");
                for(int i=0; i<list_size(instrucciones); i++){
                    logear_instruccion(logger,(t_instruccion*) list_get(instrucciones,i));
                }
                break;
            case SEGMENTOS:
                break;
            default: ;
        }
    }

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