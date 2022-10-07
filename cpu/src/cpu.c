#include "../headers/cpu.h"

int main(int argc, char* argv[]){

    validar_argumentos_main(argc);

	char* CONFIG_FILE = argv[1];

    logger = iniciar_logger(LOG_FILE,LOG_NAME);
    consola_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();

	char* IP_MEMORIA = config_get_string_value(communication_config,"IP_MEMORIA");
    int PUERTO_MEMORIA = config_get_int_value(communication_config,"PUERTO_MEMORIA");
	char* IP_KERNEL = config_get_string_value(communication_config,"IP_KERNEL");
	int PUERTO_KERNEL = config_get_int_value(communication_config,"PUERTO_KERNEL");

	char* IP_CPU = config_get_string_value(communication_config,"IP_CPU");
	char* PUERTO_DISPATCH = config_get_string_value(communication_config, "PUERTO_CPU_DISPATCH");

    int socket_server_cpu = iniciar_servidor(IP_CPU, PUERTO_DISPATCH, logger);
    int socket_kernel = esperar_cliente(socket_server_cpu, logger );
    t_pcb* pcb = recibir_PCB(socket_kernel);

   /* int socket_memoria = crear_conexion(IP_MEMORIA,PUERTO_MEMORIA);
    uint32_t respuesta_memoria = enviar_handshake_inicial(socket_memoria,CPU_DISPATCH,logger);

	int socket_kernel_dispatch = crear_conexion(IP_KERNEL,PUERTO_KERNEL);
    uint32_t respuesta_dispatch = enviar_handshake_inicial(socket_kernel_dispatch,CPU_DISPATCH,logger);

	int socket_kernel_interrupt = crear_conexion(IP_KERNEL,PUERTO_KERNEL);
    uint32_t respuesta_interrupt = enviar_handshake_inicial(socket_kernel_interrupt,CPU_INTERRUPT,logger);
*/
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