#include "../headers/memoria.h"

int main(int argc, char* argv[]){

    validar_argumentos_main(argc);
	
	char* CONFIG_FILE = argv[1];

    logger = iniciar_logger(LOG_FILE,LOG_NAME);
    consola_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();

	//Levanto el servidor de memoria
    int socket_srv_memoria = levantar_servidor();
    //Espero la conexion de la cpu
	int socket_cliente = esperar_cliente(socket_srv_memoria,logger);
    uint32_t modulo = recibir_handshake_inicial(socket_cliente,MEMORIA,logger);

	char* IP_KERNEL = config_get_string_value(communication_config,"IP_KERNEL");
	int PUERTO_KERNEL = config_get_int_value(communication_config,"PUERTO_KERNEL");

	int socket_kernel = crear_conexion(IP_KERNEL,PUERTO_KERNEL);
    uint32_t respuesta = enviar_handshake_inicial(socket_kernel,MEMORIA,logger);
	usleep(10000000);

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
    char* ip_memoria = config_get_string_value(communication_config,"IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(communication_config,"PUERTO_MEMORIA");

    return iniciar_servidor(ip_memoria,puerto_memoria,logger);
}