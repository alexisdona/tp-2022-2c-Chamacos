#include "../headers/kernel.h"
#include <unistd.h>

int main(int argc, char* argv[]) {

    validar_argumentos_main(argc);

    char* CONFIG_FILE = argv[1];

    logger = iniciar_logger(LOG_FILE, LOG_NAME);
    consola_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();

    int socket_srv_kernel = levantar_servidor();
   // esperar_modulos(socket_srv_kernel);
    esperar_consolas(socket_srv_kernel);

    log_info(logger, "### ESPERANDO CONSOLAS ###");


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

    return iniciar_servidor(ip_kernel, puerto_kernel,logger);
}

void esperar_consolas(int socket_srv){
    log_info(logger,"Esperando consolas..");
    while(1){
        pthread_t thread_escucha_consola;
        int socket_consola = esperar_cliente(socket_srv, logger);
     //   recibir_handshake_inicial(socket_consola,KERNEL,logger);
        pthread_create(&thread_escucha_consola, NULL, conexion_consola, (void*)(intptr_t) socket_consola);
        pthread_detach(thread_escucha_consola);
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
                        pthread_detach(thread_escucha_dispatch);
                        break;
            case CPU_INTERRUPT:
                        pthread_create(&thread_escucha_interrupt, NULL, conexion_interrupt, (void*) (intptr_t)socket_cliente);
                        pthread_detach(thread_escucha_interrupt);
                        break;
            case MEMORIA:
                        pthread_create(&thread_escucha_memoria, NULL, conexion_memoria, (void*) (intptr_t)socket_cliente);
                        pthread_detach(thread_escucha_memoria);
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
        op_code codigo_operacion = recibir_operacion(socket_dispatch);
		switch(codigo_operacion){
			case PCB:
			    ;
			    t_pcb* un_pcb;
				//dispatch_pcb(un_pcb, socket_dispatch);
				break;
		    case -1:
		        break;
			default:
			    perror("KERNEL -> Conexión Dispatch: Operacion desconocida");
				break;
    }
  }
}

void *conexion_interrupt(void* socket){
    int socket_interrupt = (intptr_t) socket;
    while(socket_interrupt != -1){
        op_code codigo_operacion = recibir_operacion(socket_interrupt);
    }
}

void *conexion_memoria(void* socket){
    int socket_memoria = (intptr_t) socket;
    while(socket_memoria != -1){
        op_code codigo_operacion = recibir_operacion(socket_memoria);
    }
}

void *conexion_consola(void* socket){
    int socket_consola = (intptr_t) socket;
    log_info(logger, string_from_format("socket_consola: %d", socket_consola));
    while(socket_consola != -1){
        op_code codigo_operacion = recibir_operacion(socket_consola);
        log_info(logger, string_from_format("codigo_operacion: %d", codigo_operacion));

        switch(codigo_operacion){
            case LISTA_INSTRUCCIONES:
                log_info(logger,"Recibiendo una lista de instrucciones");
                t_list* instrucciones = recibir_lista_instrucciones(socket_consola);
                for(int i=0; i<list_size(instrucciones); i++){
                    logear_instruccion(logger,(t_instruccion*) list_get(instrucciones,i));
                }
                t_pcb* pcb = crear_estructura_pcb(instrucciones);
                break;

            case SEGMENTOS:
                log_info(logger,"Recibiendo segmentos");
                int* segmentos = recibir_segmentos(socket_consola);
                for(int i=0; i<sizeof(segmentos); i++){
                    log_info(logger, string_from_format("Segmento: %d",segmentos[i]));
                }
            default: ;
        }
    }
}


t_pcb* crear_estructura_pcb(t_list* lista_instrucciones) {
    t_pcb *pcb =  malloc(sizeof(t_pcb));

    pcb->pid = getpid();
    pcb->ppid = getppid();
    pcb->registros_pcb.registro_ax=0;
    pcb->registros_pcb.registro_bx=0;
    pcb->registros_pcb.registro_cx=0;
    pcb->registros_pcb.registro_dx=0;
    pcb->datos_segmentos.indice_tabla_paginas_segmentos = 0; //TODO
    pcb->lista_instrucciones = lista_instrucciones;
    pcb->program_counter= 0;
    return pcb;
}