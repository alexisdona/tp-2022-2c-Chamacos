#include "../headers/kernel.h"

int main(int argc, char* argv[]) {

    ultimo_pid=0;

    validar_argumentos_main(argc);

    char* CONFIG_FILE = argv[1];

    logger = iniciar_logger(LOG_FILE, LOG_NAME);
    kernel_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();

    uint32_t grado_max_multiprogramacion = config_get_int_value(kernel_config,"GRADO_MAX_MULTIPROGRAMACION");

    inicializar_mutex();
    inicializar_semaforos_sincronizacion(grado_max_multiprogramacion);
    iniciar_planificacion();

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

    return iniciar_servidor(ip_kernel, puerto_kernel,logger);
}

void esperar_consolas(int socket_srv){
    log_info(logger,"Esperando consolas..");
    printf("\n");
    while(1){
        pthread_t thread_escucha_consola;
        int socket_consola = esperar_cliente(socket_srv, logger);
        recibir_handshake_inicial(socket_consola,KERNEL,logger);
        pthread_create(&thread_escucha_consola, NULL, conexion_consola, (void*)(intptr_t) socket_consola);
        pthread_detach(thread_escucha_consola);
    }
}

void esperar_modulos(int socket_srv){
    log_info(logger,"Esperando modulos..");
    printf("\n");
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

void* conexion_dispatch(void* socket){
    socket_dispatch = (intptr_t) socket;
    while(socket_dispatch != -1){
        //pthread_mutex_lock(&mutex_dispatch);
        op_code codigo_operacion = recibir_operacion(socket_dispatch);
		switch(codigo_operacion){
			case PCB:
			    ;
			    t_pcb* un_pcb;
                //recibir_pcb
                //pthread_mutex_unlock(&mutex_dispatch);
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

void* conexion_interrupt(void* socket){
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
    printf("\n");
    int socket_consola = (intptr_t) socket;
    log_info(logger, string_from_format("socket_consola: %d", socket_consola));
    while(socket_consola != -1){
        op_code codigo_operacion = recibir_operacion(socket_consola);
        log_info(logger, string_from_format("codigo_operacion: %d", codigo_operacion));

        switch(codigo_operacion){
            case LISTA_INSTRUCCIONES_SEGMENTOS:
                log_info(logger,"Recibiendo una lista de instrucciones y segmentos");
                t_list* instrucciones = recibir_lista_instrucciones(socket_consola);
                for(int i=0; i<list_size(instrucciones); i++){
                    logear_instruccion(logger,(t_instruccion*) list_get(instrucciones, i));
                }

                t_list* segmentos = recibir_lista_segmentos(socket_consola);
                for(int i=0; i<list_size(segmentos); i++){
                    printf("segmento[%d]: %d\n",i, (uint32_t) list_get(segmentos, i));
                }

                t_pcb* pcb = crear_estructura_pcb(instrucciones, segmentos);
                agregar_pcb_a_cola(pcb,mutex_new,new_queue);
                log_info(logger,string_from_format("Se crea el proceso <%d> en NEW",pcb->pid));
                sem_post(&new_to_ready);
                break;
            default:
                break;

        }
    }
}

t_pcb* crear_estructura_pcb(t_list* lista_instrucciones, t_list* segmentos) {
    t_pcb *pcb =  malloc(sizeof(t_pcb));
    pcb->pid = ultimo_pid;
    pcb->registros_pcb.registro_ax=0;
    pcb->registros_pcb.registro_bx=0;
    pcb->registros_pcb.registro_cx=0;
    pcb->registros_pcb.registro_dx=0;
    pcb->datos_segmentos.indice_tabla_paginas_segmentos = 0;
    pcb->datos_segmentos.segmentos = segmentos;
    pcb->lista_instrucciones = lista_instrucciones;
    pcb->program_counter= 0;
    ultimo_pid++;

    return pcb;
}

void iniciar_planificacion(){
    log_info(logger,"Iniciando planificadores..");
    pthread_t dispatcher;
    pthread_t long_planner;

    new_queue = queue_create();
    exit_queue = queue_create();
    ready_queue = queue_create();
    blocked_screen_queue = queue_create();
    blocked_keyboard_queue = queue_create();
    blocked_page_fault_queue = queue_create();
    blocked_io_queue = queue_create();
    running_queue = queue_create();
    exit_queue = queue_create();

    pthread_create(&long_planner, NULL, planificador_largo_plazo, NULL);
    pthread_detach(long_planner);
    pthread_create(&dispatcher, NULL, planificador_corto_plazo, NULL);
    pthread_detach(dispatcher);
}

void inicializar_mutex(){
    pthread_mutex_init(&mutex_new,NULL);
    pthread_mutex_init(&mutex_ready,NULL);
    pthread_mutex_init(&mutex_running,NULL);
    pthread_mutex_init(&mutex_blocked_screen,NULL);
    pthread_mutex_init(&mutex_blocked_keyboard,NULL);
    pthread_mutex_init(&mutex_blocked_page_fault,NULL);
    pthread_mutex_init(&mutex_blocked_io,NULL);
    pthread_mutex_init(&mutex_exit,NULL);
    pthread_mutex_init(&mutex_dispatch,NULL);
    pthread_mutex_init(&mutex_interrupt,NULL);
    pthread_mutex_init(&mutex_memoria,NULL);
}

void inicializar_semaforos_sincronizacion(uint32_t multiprogramacion){
    sem_init(&grado_multiprogramacion,0,multiprogramacion);
    sem_init(&new_process,0,0);
    sem_init(&new_to_ready,0,0);
    sem_init(&ready_to_running,0,0);
    sem_init(&cpu_libre,0,1);
    sem_init(&finish_process,0,0);
}

void agregar_pcb_a_cola(t_pcb* pcb,pthread_mutex_t mutex, t_queue* cola){
    pthread_mutex_lock(&mutex);
    queue_push(cola,pcb);
    pthread_mutex_unlock(&mutex);
}

t_pcb* quitar_pcb_de_cola(pthread_mutex_t mutex, t_queue* cola){
    pthread_mutex_lock(&mutex);
    t_pcb* pcb = queue_pop(cola);
    pthread_mutex_unlock(&mutex);
    return pcb;
}

void* planificador_largo_plazo(void* x){
    log_info(logger,"Iniciando Planificador Largo plazo..");
    pthread_t process_terminator_thread;
    pthread_create(&process_terminator_thread, NULL, finalizador_procesos, NULL);
    pcb_a_dispatcher();
    pthread_join(process_terminator_thread,NULL);
}

void pcb_a_dispatcher(){
    while(1){
        sem_wait(&new_to_ready);
        sem_wait(&grado_multiprogramacion);
        t_pcb* pcb = quitar_pcb_de_cola(mutex_new,new_queue);
        agregar_pcb_a_cola(pcb,mutex_ready,ready_queue);
        log_info(logger,string_from_format("PID: <%d> - Estado Anterior <NEW>   - Estado Actual <READY>",pcb->pid));
        sem_post(&ready_to_running);
    }
}

void* finalizador_procesos(void* x){
    while(1){
        sem_wait(&finish_process);
        t_pcb* pcb = quitar_pcb_de_cola(mutex_exit,exit_queue);
        free(pcb);
        //Avisar a memoria
        //Avisar a consola
    }
}

void* planificador_corto_plazo(void* x){
    log_info(logger,"Iniciando Planificador Corto plazo..");
    pthread_t ready_thread;
    pthread_t blocked_page_fault_thread;
    pthread_t blocked_io_thread;
    pthread_t blocked_keyboard_thread;
    pthread_t blocked_screen_thread;
    pthread_t running_thread;

    pthread_create(&ready_thread, NULL, manejador_estado_ready, NULL);
    pthread_create(&running_thread, NULL, manejador_estado_running, NULL);

    pthread_join(ready_thread,NULL);

}

void* manejador_estado_ready(void* x){
    while(1){
        sem_wait(&ready_to_running);
        t_pcb* pcb = quitar_pcb_de_cola(mutex_ready,ready_queue);
        agregar_pcb_a_cola(pcb,mutex_running,running_queue);
        log_info(logger,string_from_format("PID: <%d> - Estado Anterior <READY> - Estado Actual <RUNNING>",pcb->pid));
    }
}

void* manejador_estado_running(void* x){
    while(1){
        sem_wait(&cpu_libre);
        //Enviar pcb a cpu
    }
}