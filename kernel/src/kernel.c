#include "../headers/kernel.h"

int main(int argc, char* argv[]) {

    ultimo_pid=0;
    INTERRUPCIONES_HABILITADAS = 1;

    validar_argumentos_main(argc);

    char* CONFIG_FILE = argv[1];

    logger = iniciar_logger(LOG_FILE, LOG_NAME);
    kernel_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();

    uint32_t grado_max_multiprogramacion = config_get_int_value(kernel_config,"GRADO_MAX_MULTIPROGRAMACION");
    char** lista_bloqueos = config_get_array_value(kernel_config,"TIEMPOS_IO");

    tiempos_bloqueos = malloc(string_array_size(lista_bloqueos)*sizeof(uint32_t));

    for(int i=0; i<string_array_size(lista_bloqueos); i++){
        tiempos_bloqueos[i] = atoi(lista_bloqueos[i]);
    }

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
    pthread_mutex_lock(&mutex_logger);
    log_info(logger,"Esperando consolas..");
    pthread_mutex_unlock(&mutex_logger);
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
    pthread_mutex_lock(&mutex_logger);
    log_info(logger,"Esperando modulos..");
    pthread_mutex_unlock(&mutex_logger);
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
                pthread_mutex_lock(&mutex_logger);
                log_warning(logger,"Modulo desconocido");
                pthread_mutex_unlock(&mutex_logger);
                printf(YEL"\t > Identificador Modulo: %d\n"WHT,modulo);
                i--;
        }
    }
}

void* conexion_dispatch(void* socket){
    int socket_dispatch = (intptr_t) socket;
    t_pcb* pcb;
    uint32_t interrupciones_iniciadas = INTERRUPCIONES_HABILITADAS;

    while(socket_dispatch != -1){

        sem_wait(&enviar_pcb_a_cpu);
        pcb = quitar_pcb_de_cola(mutex_running,running_queue);
        
        enviar_PCB(socket_dispatch,pcb,PCB);
        pthread_mutex_lock(&mutex_logger);
        log_info(logger,string_from_format(BLU"Conexion Dispatch: PCB Enviado: PID <%d>"WHT,pcb->pid));
        pthread_mutex_unlock(&mutex_logger);
        hay_proceso_ejecutando=1;
        sem_post(&continuar_conteo_quantum);

        if(algoritmo_planificacion_tiene_desalojo() && interrupciones_iniciadas == INTERRUPCIONES_HABILITADAS){
            interrupciones_iniciadas--;
        }

        op_code codigo_operacion = recibir_operacion(socket_dispatch);
        pcb = recibir_PCB(socket_dispatch);
        hay_proceso_ejecutando=0;
        pthread_mutex_lock(&mutex_logger);
        log_info(logger,string_from_format(BLU"Conexion Dispatch: PCB Recibido: PID <%d>"WHT,pcb->pid));
        pthread_mutex_unlock(&mutex_logger);

        switch(codigo_operacion){
            case BLOQUEAR_PROCESO_IO:
                if(algoritmo_planificacion_tiene_desalojo()) pthread_cancel(thread_clock);
                motivo_bloqueo = codigo_operacion;
                agregar_pcb_a_cola(pcb,mutex_blocked_io,blocked_io_queue);
                logear_cambio_estado(pcb,RUNNING,BLOQUEADO_IO);
                sem_post(&bloquear_por_io);
                break;

            case BLOQUEAR_PROCESO_TECLADO:
                if(algoritmo_planificacion_tiene_desalojo()) pthread_cancel(thread_clock);
                motivo_bloqueo = codigo_operacion;
                agregar_pcb_a_cola(pcb,mutex_blocked_keyboard,blocked_keyboard_queue);
                logear_cambio_estado(pcb,RUNNING,BLOQUEADO_TECLADO);
                sem_post(&bloquear_por_teclado);
                break;

            case BLOQUEAR_PROCESO_PANTALLA:
                if(algoritmo_planificacion_tiene_desalojo()) pthread_cancel(thread_clock);
                motivo_bloqueo = codigo_operacion;
                agregar_pcb_a_cola(pcb,mutex_blocked_screen,blocked_screen_queue);
                logear_cambio_estado(pcb,RUNNING,BLOQUEADO_PANTALLA);
                sem_post(&bloquear_por_pantalla);
                break;

            case PAGE_FAULT:
                if(algoritmo_planificacion_tiene_desalojo()) pthread_cancel(thread_clock);
                motivo_bloqueo = codigo_operacion;
                agregar_pcb_a_cola(pcb,mutex_blocked_page_fault,blocked_page_fault_queue);
                logear_cambio_estado(pcb,RUNNING,BLOQUEADO_PAGE_FAULT);
                sem_post(&bloquear_por_pf);
                break;

            case FINALIZAR_PROCESO:
                if(algoritmo_planificacion_tiene_desalojo()) pthread_cancel(thread_clock);
                agregar_pcb_a_cola(pcb,mutex_exit,exit_queue);
                logear_cambio_estado(pcb,RUNNING,EXIT_S);
                sem_post(&finish_process);
                break;

            case INTERRUPCION:
                agregar_a_ready(pcb,INTERRUPCION,RUNNING);
                break;

            default:
                pthread_mutex_lock(&mutex_logger);
                log_warning(logger,"Conexion Dispatch -> Recibio una operacion incorrecta");
                pthread_mutex_unlock(&mutex_logger);
                printf(YEL"\t > Codigo Operacion: %d\n"WHT,codigo_operacion);
                exit(EXIT_FAILURE);
        }
        sem_post(&cpu_libre);
    }
    return EXIT_SUCCESS;
}

void* conexion_interrupt(void* socket){
    int socket_interrupt = (intptr_t) socket;
    if(algoritmo_planificacion_tiene_desalojo()){
        pthread_mutex_lock(&mutex_logger);
        log_info(logger,string_from_format("Quantum: %ds",quantum/1000));
        pthread_mutex_unlock(&mutex_logger);
        printf("\n");
        quantum = quantum * 1000;
        while(socket_interrupt != -1){
            sem_wait(&continuar_conteo_quantum);
            pthread_create(&thread_clock, NULL, clock_interrupt, (void*) (uint32_t)socket_interrupt);
            pthread_detach(thread_clock);
        }
    }
    return EXIT_SUCCESS;
}

void* clock_interrupt(void* socket){
    int socket_interrupt = (intptr_t) socket;
    printf("\n");
    pthread_mutex_lock(&mutex_logger);
    log_info(logger,"Clock iniciado...");
    pthread_mutex_unlock(&mutex_logger);
    usleep(quantum);
    if(hay_proceso_ejecutando) {
        pthread_mutex_lock(&mutex_logger);
        log_info(logger,BLU"Conexion Interrupt: CLOCK - Enviando Interrupcion a CPU"WHT);
        pthread_mutex_unlock(&mutex_logger);
        enviar_interrupcion(socket_interrupt);
    }
    return EXIT_SUCCESS;
}

void *conexion_memoria(void* socket){
    int socket_memoria = (intptr_t) socket;
    while(socket_memoria != -1){
        op_code codigo_operacion = recibir_operacion(socket_memoria);
    }
    return EXIT_SUCCESS;
}

void *conexion_consola(void* socket){
    printf("\n");
    int socket_consola = (intptr_t) socket;
    while(socket_consola != -1){
        op_code codigo_operacion = recibir_operacion(socket_consola);

        switch(codigo_operacion){
            case LISTA_INSTRUCCIONES_SEGMENTOS:
                pthread_mutex_lock(&mutex_logger);
                log_info(logger,"Recibiendo una lista de instrucciones y segmentos");
                pthread_mutex_unlock(&mutex_logger);
                t_list* instrucciones = recibir_lista_instrucciones(socket_consola);
                for(int i=0; i<list_size(instrucciones); i++){
                    logear_instruccion(logger,(t_instruccion*) list_get(instrucciones, i));
                }
                t_list* segmentos = recibir_lista_segmentos(socket_consola);

                pthread_mutex_lock(&mutex_pid);
                t_pcb* pcb = crear_estructura_pcb(instrucciones, segmentos, socket_consola);
                pthread_mutex_unlock(&mutex_pid);
                agregar_pcb_a_cola(pcb,mutex_new,new_queue);
                pthread_mutex_lock(&mutex_logger);
                log_info(logger,string_from_format(CYN"Se crea el proceso <%d> en NEW"WHT,pcb->pid));
                pthread_mutex_unlock(&mutex_logger);
                sem_post(&new_to_ready);
                break;
            
			case INPUT_VALOR:
				valor_consola = recibir_valor(socket_consola);
				pthread_mutex_lock(&mutex_logger);
				log_info(logger,string_from_format(BLU"Valor recibido de consola: %d"BLU,valor_consola));
				pthread_mutex_unlock(&mutex_logger);
				break;

            case IMPRIMIR_VALOR:
                sem_post(&desbloquear_pantalla);
                break;
                
            case -1:
                return EXIT_SUCCESS;

            default:
                pthread_mutex_lock(&mutex_logger);
                log_warning(logger,"Conexion Consola -> Recibio una operacion incorrecta");
                pthread_mutex_unlock(&mutex_logger);
                printf(YEL"\t > Codigo Operacion: %d\n"WHT,codigo_operacion);
                exit(EXIT_FAILURE);
        }
    }
    return EXIT_SUCCESS;
}

t_pcb* crear_estructura_pcb(t_list* lista_instrucciones, t_list* segmentos, uint32_t socket_consola) {
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
    pcb->socket_consola = socket_consola;
    ultimo_pid++;

    return pcb;
}

void iniciar_planificacion(){
    pthread_mutex_lock(&mutex_logger);
    log_info(logger,"Iniciando planificadores..");
    pthread_mutex_unlock(&mutex_logger);

    algoritmo_planificacion = config_get_string_value(kernel_config,"ALGORITMO_PLANIFICACION");
    quantum = config_get_int_value(kernel_config,"QUANTUM_RR");

    pthread_t dispatcher;
    pthread_t long_planner;

    new_queue = queue_create();
    exit_queue = queue_create();
    ready1_queue = queue_create();
    if(string_contains(algoritmo_planificacion,"FEEDBACK")){
        ready2_queue = queue_create();
    }
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
    pthread_mutex_init(&mutex_logger,NULL);
    pthread_mutex_init(&mutex_new,NULL);
    pthread_mutex_init(&mutex_ready1,NULL);
    pthread_mutex_init(&mutex_ready2,NULL);
    pthread_mutex_init(&mutex_running,NULL);
    pthread_mutex_init(&mutex_blocked_screen,NULL);
    pthread_mutex_init(&mutex_blocked_keyboard,NULL);
    pthread_mutex_init(&mutex_blocked_page_fault,NULL);
    pthread_mutex_init(&mutex_blocked_io,NULL);
    pthread_mutex_init(&mutex_exit,NULL);
    pthread_mutex_init(&mutex_pid,NULL);
}

void inicializar_semaforos_sincronizacion(uint32_t multiprogramacion){
    sem_init(&grado_multiprogramacion,0,multiprogramacion);
    sem_init(&new_process,0,0);
    sem_init(&new_to_ready,0,0);
    sem_init(&pcbs_en_ready,0,0);
    sem_init(&cpu_libre,0,1);
    sem_init(&finish_process,0,0);
    sem_init(&continuar_conteo_quantum,0,0);
    sem_init(&enviar_pcb_a_cpu,0,0);
    sem_init(&redirigir_proceso_bloqueado,0,0);
    sem_init(&bloquear_por_io,0,0);
    sem_init(&bloquear_por_pantalla,0,0);
    sem_init(&desbloquear_pantalla,0,0);
    sem_init(&bloquear_por_teclado,0,0);
    sem_init(&bloquear_por_pf,0,0);
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

int algoritmo_planificacion_tiene_desalojo(){
    return !string_equals_ignore_case(algoritmo_planificacion,"FIFO");
}

void* planificador_largo_plazo(void* x){
    pthread_mutex_lock(&mutex_logger);
    log_info(logger,"Iniciando Planificador Largo plazo..");
    pthread_mutex_unlock(&mutex_logger);
    pthread_t process_terminator_thread;
    pthread_create(&process_terminator_thread, NULL, finalizador_procesos, NULL);
    pcb_a_dispatcher();
    pthread_join(process_terminator_thread,NULL);
    return EXIT_SUCCESS;
}

void pcb_a_dispatcher(){
    while(1){
        sem_wait(&new_to_ready);
        sem_wait(&grado_multiprogramacion);
        t_pcb* pcb = quitar_pcb_de_cola(mutex_new,new_queue);
        agregar_a_ready(pcb,PCB,NEW);
    }
}

void* finalizador_procesos(void* x){
    while(1){
        sem_wait(&finish_process);
        t_pcb* pcb = quitar_pcb_de_cola(mutex_exit,exit_queue);

        enviar_codigo_op(pcb->socket_consola,FINALIZAR_PROCESO);
        sem_post(&grado_multiprogramacion);
        //free(pcb);
        //Avisar a memoria
    }
    return EXIT_SUCCESS;
}

void* planificador_corto_plazo(void* x){
    pthread_mutex_lock(&mutex_logger);
    log_info(logger,"Iniciando Planificador Corto plazo..");
    pthread_mutex_unlock(&mutex_logger);
    pthread_t ready_thread;
    pthread_t blocked_page_fault_thread;
    pthread_t blocked_io_thread;
    pthread_t blocked_keyboard_thread;
    pthread_t blocked_screen_thread;

    pthread_create(&ready_thread, NULL, manejador_estado_ready, NULL);
    pthread_create(&blocked_page_fault_thread, NULL, manejador_estado_blocked_pf, NULL);
    pthread_create(&blocked_io_thread, NULL, manejador_estado_blocked_io, NULL);
    pthread_create(&blocked_screen_thread, NULL, manejador_estado_blocked_screen, NULL);
    pthread_create(&blocked_keyboard_thread, NULL, manejador_estado_blocked_keyboard, NULL);

    pthread_join(ready_thread,NULL);

    return EXIT_SUCCESS;
}

void* manejador_estado_ready(void* x){
    estado_pcb ready_anterior;
    while(1){
        sem_wait(&pcbs_en_ready);
        sem_wait(&cpu_libre);
        t_pcb* pcb = quitar_de_ready(&ready_anterior);
        agregar_pcb_a_cola(pcb,mutex_running,running_queue);
        logear_cambio_estado(pcb,ready_anterior,RUNNING);
        sem_post(&enviar_pcb_a_cpu);
    }
    return EXIT_SUCCESS;
}

void* manejador_estado_blocked_pf(void* x){
    sem_wait(&bloquear_por_pf);

}

void* manejador_estado_blocked_io(void* x){
    uint32_t tiempo_bloqueado;
    dispositivo io;
    t_pcb* pcb;

    while(1){
        sem_wait(&bloquear_por_io);
        pcb = quitar_pcb_de_cola(mutex_blocked_io,blocked_io_queue);
        obtener_dispositivo_tiempo_bloqueo(pcb,&io,&tiempo_bloqueado);
        pthread_mutex_lock(&mutex_logger);
        log_info(logger,string_from_format(CYN"PID: <%d> - Bloqueado por: <%s> durante %ds"WHT,pcb->pid,traducir_dispositivo(io),tiempo_bloqueado/1000000));
        pthread_mutex_unlock(&mutex_logger);
        usleep(tiempo_bloqueado);
        agregar_a_ready(pcb,BLOQUEAR_PROCESO_IO,BLOQUEADO_IO);
    }
    return EXIT_SUCCESS;
}

void* manejador_estado_blocked_screen(void* x){
    t_pcb* pcb;

    while(1){
        sem_wait(&bloquear_por_pantalla);
        pcb = quitar_pcb_de_cola(mutex_blocked_screen,blocked_screen_queue);
        pthread_mutex_lock(&mutex_logger);
        log_info(logger,string_from_format(CYN"PID: <%d> - Bloqueado por: <PANTALLA>"WHT,pcb->pid));
        pthread_mutex_unlock(&mutex_logger);
        uint32_t valor_a_imprimir = obtener_valor_registro_por_bloqueo_pantalla(pcb);
        enviar_imprimir_valor(valor_a_imprimir,pcb->socket_consola);
        sem_wait(&desbloquear_pantalla);
        agregar_a_ready(pcb,BLOQUEAR_PROCESO_PANTALLA,BLOQUEADO_PANTALLA);
    }
}

void* manejador_estado_blocked_keyboard(void* x){
        sem_wait(&bloquear_por_teclado);

}

void obtener_dispositivo_tiempo_bloqueo(t_pcb* pcb, dispositivo* disp, uint32_t* tiempo_bloqueo){
    t_instruccion* instruccion_anterior = obtener_instruccion_anterior(pcb);
    *disp = instruccion_anterior->parametros[0];
    *tiempo_bloqueo = tiempos_bloqueos[*disp] * instruccion_anterior->parametros[1] * 1000;
}

uint32_t obtener_valor_registro_por_bloqueo_pantalla(t_pcb* pcb){
    t_instruccion* instruccion_anterior = obtener_instruccion_anterior(pcb);
    registro_cpu registro = instruccion_anterior->parametros[1];
    return *obtener_registro(pcb, registro);
}



t_instruccion* obtener_instruccion_anterior(t_pcb* pcb){
    t_instruccion* instruccion_anterior = list_get(pcb->lista_instrucciones,pcb->program_counter-1);
    return instruccion_anterior;
}

void agregar_a_ready(t_pcb* pcb, op_code motivo, estado_pcb anterior){
    if(algoritmo_es_feedback() && motivo == INTERRUPCION){
        logear_cambio_estado(pcb,anterior,READY2);
        agregar_pcb_a_cola(pcb,mutex_ready2,ready2_queue);
    }else{
        logear_cambio_estado(pcb,anterior,READY1);
        agregar_pcb_a_cola(pcb,mutex_ready1,ready1_queue);
    }
    sem_post(&pcbs_en_ready);
}

t_pcb* quitar_de_ready(estado_pcb* ready){
    if(!queue_is_empty(ready1_queue)){
        *ready = READY1;
        return quitar_pcb_de_cola(mutex_ready1,ready1_queue);
    }else{
        if(algoritmo_es_feedback() && !queue_is_empty(ready2_queue)){
            *ready = READY2;
            return quitar_pcb_de_cola(mutex_ready2,ready2_queue);
        }
    }
    pthread_mutex_lock(&mutex_logger);
    log_error(logger,"Se intento quitar un pcb de alguna cola de READY pero ambas estan vacias");
    pthread_mutex_unlock(&mutex_logger);
    return EXIT_SUCCESS;
}

int algoritmo_es_feedback(){
    return string_equals_ignore_case(algoritmo_planificacion,"FEEDBACK");
}

void logear_cambio_estado(t_pcb* pcb,estado_pcb anterior,estado_pcb actual){
    const char* log = string_from_format(CYN"PID: <%d> - Estado Anterior <%s> - Estado Actual <%s>"WHT,pcb->pid,traducir_estado_pcb(anterior),traducir_estado_pcb(actual));
    pthread_mutex_lock(&mutex_logger);
    log_info(logger,log);
    pthread_mutex_unlock(&mutex_logger);
}

char* traducir_estado_pcb(estado_pcb estado){
    switch(estado){
        case NEW:                   return "NEW";
        case READY1:                return "READY1";
        case READY2:                return "READY2";
        case RUNNING:               return "RUNNING";
        case BLOQUEADO_IO:          return "BLOQUEADO_IO";
        case BLOQUEADO_PANTALLA:    return "BLOQUEADO_PANTALLA";
        case BLOQUEADO_TECLADO:     return "BLOQUEADO_TECLADO";
        case BLOQUEADO_PAGE_FAULT:  return "BLOQUEADO_PAGE_FAULT";
        case EXIT_S:                return "EXIT";
        default:                    return "ERROR - NOMBRE ESTADO INADECUADO";
    }
}