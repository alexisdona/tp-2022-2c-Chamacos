#include "../headers/kernel.h"

int main(int argc, char* argv[]) {

    ultimo_pid=1;
    INTERRUPCIONES_HABILITADAS = 1;

    validar_argumentos_main(argc);

    char* CONFIG_FILE = argv[1];

    logger = iniciar_logger(LOG_FILE, LOG_NAME);
    kernel_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();

    uint32_t grado_max_multiprogramacion = config_get_int_value(kernel_config,"GRADO_MAX_MULTIPROGRAMACION");
    lista_dispositivos = config_get_array_value(kernel_config,"DISPOSITIVOS_IO");
    char** lista_bloqueos = config_get_array_value(kernel_config,"TIEMPOS_IO");
    cantidad_dispositivos = string_array_size(lista_bloqueos);

    tiempos_bloqueos = malloc(cantidad_dispositivos*sizeof(uint32_t));

    for(int i=0; i<cantidad_dispositivos; i++){
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
        //recibir_handshake_inicial(socket_consola,KERNEL,logger);
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
        uint32_t modulo = recibir_handshake_inicial(socket_cliente,KERNEL, logger);
        log_info(logger, "Se conecto un cliente en kernel: %d, modulo:%d", socket_cliente, modulo);

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
        /*
        pthread_mutex_lock(&mutex_logger);
        log_info(logger,string_from_format(BLU"Conexion Dispatch: PCB Enviado: PID <%d>"WHT,pcb->pid));
        pthread_mutex_unlock(&mutex_logger);
        */
        hay_proceso_ejecutando=1;

        if(algoritmo_planificacion_tiene_desalojo() && ready_anterior_pcb_running==READY1) sem_post(&continuar_conteo_quantum);

        if(algoritmo_planificacion_tiene_desalojo() && interrupciones_iniciadas == INTERRUPCIONES_HABILITADAS){
            interrupciones_iniciadas--;
        }

        op_code codigo_operacion = recibir_operacion(socket_dispatch);
        pcb = recibir_PCB(socket_dispatch);
        hay_proceso_ejecutando=0;
        /*
        pthread_mutex_lock(&mutex_logger);
        log_info(logger,string_from_format(BLU"Conexion Dispatch: PCB Recibido: PID <%d>"WHT,pcb->pid));
        pthread_mutex_unlock(&mutex_logger);
        */
        switch(codigo_operacion){

            case INTERRUPCION:
                pthread_mutex_lock(&mutex_logger);
                log_info(logger,string_from_format(GRN"PID: <%d> - Desalojado por Fin de Quantum"WHT,pcb->pid));
                pthread_mutex_unlock(&mutex_logger);
                agregar_a_ready(pcb,INTERRUPCION,RUNNING);
                break;

            case BLOQUEAR_PROCESO_IO:
                if(algoritmo_planificacion_tiene_desalojo()) pthread_cancel(thread_clock);
                dispositivo io = obtener_dispositivo(pcb);
                logear_cambio_estado(pcb,RUNNING,BLOQUEADO_IO);
                agregar_pcb_a_lista(pcb,mutex_blocked_io,blocked_io_list);
                sem_t* sem = list_get(semaforos_dispositivos,io);
                sem_post(sem);
                break;

            case BLOQUEAR_PROCESO_PANTALLA:
                if(algoritmo_planificacion_tiene_desalojo()) pthread_cancel(thread_clock);
                agregar_pcb_a_lista(pcb,mutex_blocked_screen,blocked_screen_list);
                logear_cambio_estado(pcb,RUNNING,BLOQUEADO_PANTALLA);
                sem_post(&bloquear_por_pantalla);
                break;

            case BLOQUEAR_PROCESO_TECLADO:
                if(algoritmo_planificacion_tiene_desalojo()) pthread_cancel(thread_clock);
                agregar_pcb_a_lista(pcb,mutex_blocked_keyboard,blocked_keyboard_list);
                logear_cambio_estado(pcb,RUNNING,BLOQUEADO_TECLADO);
                sem_post(&bloquear_por_teclado);
                break;

            case PAGE_FAULT:
                if(algoritmo_planificacion_tiene_desalojo()) pthread_cancel(thread_clock);
                agregar_pcb_a_cola(pcb,mutex_blocked_page_fault,blocked_page_fault_queue);
                logear_cambio_estado(pcb,RUNNING,BLOQUEADO_PAGE_FAULT);
                pthread_mutex_lock(&mutex_logger);
                log_info(logger,string_from_format(CYN"PID: <%d> - Bloqueado por: <PAGE_FAULT>"WHT,pcb->pid));
                pthread_mutex_unlock(&mutex_logger);
                sem_post(&atender_pf);
                break;

            case FINALIZAR_PROCESO:
                if(algoritmo_planificacion_tiene_desalojo()) pthread_cancel(thread_clock);
                agregar_pcb_a_cola(pcb,mutex_exit, exit_queue);
                logear_cambio_estado(pcb,RUNNING,EXIT_S);
                sem_post(&finish_process);
                break;

            case SEGMENTATION_FAULT:
                if(algoritmo_planificacion_tiene_desalojo()) pthread_cancel(thread_clock);
                agregar_pcb_a_cola(pcb,mutex_exit, exit_queue);
                pthread_mutex_lock(&mutex_logger);
                log_info(logger,string_from_format(RED" <%d> SEGMENTATION FAULT",pcb->pid));
                pthread_mutex_unlock(&mutex_logger);
                logear_cambio_estado(pcb,RUNNING,EXIT_S);
                sem_post(&finish_process);
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
        /*
        pthread_mutex_lock(&mutex_logger);
        log_info(logger,string_from_format("Quantum: %ds",quantum/1000));
        pthread_mutex_unlock(&mutex_logger);
        */
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
    /*
    pthread_mutex_lock(&mutex_logger);
    log_info(logger,"Clock iniciado...");
    pthread_mutex_unlock(&mutex_logger);
    */
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
    socket_memoria = (intptr_t) socket;
    //log_info(logger, "socket_memoria: %d", socket_memoria);
    while(socket_memoria != -1) {
        op_code codigo_operacion = recibir_operacion(socket_memoria);
        switch(codigo_operacion) {
            case ACTUALIZAR_INDICE_TABLA_PAGINAS:
                ;
                t_pcb *pcb = recibir_PCB(socket_memoria);
                agregar_a_ready(pcb,PCB,NEW);
                sem_post(&estructuras_administrativas_pcb_listas);
                break;
            case PAGE_FAULT_ATENDIDO:
                ;
                sem_wait(&atender_pf);
                sem_post(&desbloquear_por_pf);
                break;

            default:
                break;
        }
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
                agregar_pcb_a_cola(pcb, mutex_new, new_queue);
                pthread_mutex_lock(&mutex_logger);
                log_info(logger,string_from_format(CYN"Se crea el proceso <%d> en NEW"WHT, pcb->pid));
                pthread_mutex_unlock(&mutex_logger);
                sem_post(&new_to_ready);
                break;

			case INPUT_VALOR:
                ;
				uint32_t input_consola = recibir_valor(socket_consola);
				/*
                pthread_mutex_lock(&mutex_logger);
				log_info(logger,string_from_format(BLU"Conexion Consola: Valor recibido de consola: %d"BLU,input_consola));
				pthread_mutex_unlock(&mutex_logger);
                */
                desbloquear_pcb_keyboard(socket_consola,input_consola);
				break;

            case IMPRIMIR_VALOR:
                log_info(logger,BLU"Conexion Consola: Valor impreso."WHT);
                desbloquear_pcb_screen(socket_consola);
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

t_pcb* crear_estructura_pcb(t_list* lista_instrucciones, t_list* tabla_segmentos, uint32_t socket_consola) {
    t_pcb *pcb =  malloc(sizeof(t_pcb));
    pcb->pid = ultimo_pid;
        
    t_registros_pcb* registros_pcb = malloc(sizeof (t_registros_pcb));

    registros_pcb->registro_ax=0;
    registros_pcb->registro_bx =0;
    registros_pcb->registro_cx =0;
    registros_pcb->registro_dx =0;

    pcb->registros_pcb = registros_pcb;

    pcb->tabla_segmentos = tabla_segmentos;
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
    ready1_list = list_create();
    if(string_contains(algoritmo_planificacion,"FEEDBACK")){
        ready2_list = list_create();
    }
    blocked_page_fault_queue = queue_create();
    blocked_io_list = list_create();
    blocked_screen_list = list_create();
    blocked_keyboard_list = list_create();
    running_queue = queue_create();
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
    pthread_mutex_init(&mutex_blocked_io,NULL);
    pthread_mutex_init(&mutex_blocked_screen,NULL);
    pthread_mutex_init(&mutex_blocked_keyboard,NULL);
    pthread_mutex_init(&mutex_blocked_page_fault,NULL);
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
    sem_init(&bloquear_por_pantalla,0,0);
    sem_init(&bloquear_por_teclado,0,0);
    sem_init(&desbloquear_por_pf,0,0);
    sem_init(&atender_pf,0,0);
    sem_init(&estructuras_administrativas_pcb_listas,0,1);

    semaforos_dispositivos = list_create();

    for(uint32_t i=0; i<cantidad_dispositivos; i++){
        sem_t* semaforo = malloc(sizeof(sem_t));
        sem_init(semaforo,0,0);
        list_add(semaforos_dispositivos,semaforo);
    }
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

void agregar_pcb_a_lista(t_pcb* pcb,pthread_mutex_t mutex, t_list* cola){
    pthread_mutex_lock(&mutex);
    list_add(cola,pcb);
    pthread_mutex_unlock(&mutex);
}

t_pcb* quitar_primer_pcb_de_lista(pthread_mutex_t mutex, t_list* cola){
    pthread_mutex_lock(&mutex);
    t_pcb* pcb = list_remove(cola,0);
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
        enviar_PCB(socket_memoria, pcb, CREAR_ESTRUCTURAS_ADMIN);
    }
}

void* finalizador_procesos(void* x){
    while(1){
        sem_wait(&finish_process);
        t_pcb* pcb = quitar_pcb_de_cola(mutex_exit,exit_queue);

        log_info(logger,BLU"Conexion Consola: Finalizar consola"WHT);
        enviar_codigo_op(pcb->socket_consola,FINALIZAR_PROCESO);
        enviar_PCB(socket_memoria,pcb,FINALIZAR_PROCESO);
        free(pcb);
        sem_post(&grado_multiprogramacion);
    }
    return EXIT_SUCCESS;
}

void* planificador_corto_plazo(void* x){
    pthread_mutex_lock(&mutex_logger);
    log_info(logger,"Iniciando Planificador Corto plazo..");
    pthread_mutex_unlock(&mutex_logger);
    pthread_t ready_thread;
    pthread_t blocked_page_fault_thread;
    pthread_t blocked_keyboard_thread;
    pthread_t blocked_screen_thread;

    pthread_create(&ready_thread, NULL, manejador_estado_ready, NULL);
    pthread_create(&blocked_page_fault_thread, NULL, manejador_estado_blocked_pf, NULL);

    for(uint32_t i=0; i<cantidad_dispositivos; i++){
        pthread_t blocked_thread_n;
        pthread_create(&blocked_thread_n, NULL, bloquear_pcb, (void*)(intptr_t) i);
        pthread_detach(blocked_thread_n);
    }

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
        ready_anterior_pcb_running = ready_anterior;
        sem_post(&enviar_pcb_a_cpu);
    }
    return EXIT_SUCCESS;
}

void* manejador_estado_blocked_pf(void* x){
    while(1) {
        sem_wait(&desbloquear_por_pf);
        t_pcb *pcb_page_fault_atendido = quitar_pcb_de_cola(mutex_blocked_page_fault, blocked_page_fault_queue);
        agregar_a_ready(pcb_page_fault_atendido, PCB, BLOQUEADO_PAGE_FAULT);
    }
}

void* bloquear_pcb(void* indice){
    dispositivo io = (intptr_t) indice;
    uint32_t tiempo_bloqueado;
    t_pcb* pcb;
    sem_t* semaforo = list_get(semaforos_dispositivos,io);

    while(1){
        sem_wait(semaforo);
        //printf("Hilo: %s\n\n",traducir_dispositivo(io));
        pthread_mutex_lock(&mutex_blocked_io);
        pcb = buscar_pcb_a_bloquear(io);
        pthread_mutex_unlock(&mutex_blocked_io);

        tiempo_bloqueado = obtener_tiempo_bloqueo(pcb);

        pthread_mutex_lock(&mutex_logger);
        float temp = tiempo_bloqueado/1000000.0;
        log_info(logger,string_from_format(CYN"PID: <%d> - Bloqueado por: <%s> durante %.1fs"WHT,pcb->pid,traducir_dispositivo(io),temp));
        pthread_mutex_unlock(&mutex_logger);
        usleep(tiempo_bloqueado);
        agregar_a_ready(pcb,BLOQUEAR_PROCESO_IO,BLOQUEADO_IO);
    }
}

t_pcb* buscar_pcb_a_bloquear(dispositivo io){
    t_pcb* pcb;
    for(uint32_t i=0; i<list_size(blocked_io_list); i++){
        pcb = list_get(blocked_io_list,i);
        dispositivo disp = obtener_dispositivo(pcb);
        if(disp == io) {
            pcb = list_remove(blocked_io_list,i);
            i=list_size(blocked_io_list);
        }
    }
    return pcb;
}

void* manejador_estado_blocked_screen(void* x){
    t_pcb* pcb;

    while(1){
        sem_wait(&bloquear_por_pantalla);
        pthread_mutex_lock(&mutex_blocked_screen);
        pcb = list_get(blocked_screen_list,list_size(blocked_screen_list)-1);
        pthread_mutex_unlock(&mutex_blocked_screen);

        pthread_mutex_lock(&mutex_logger);
        log_info(logger,string_from_format(CYN"PID: <%d> - Bloqueado por: <PANTALLA>"WHT,pcb->pid));
        pthread_mutex_unlock(&mutex_logger);
        uint32_t valor_a_imprimir = *obtener_registro_por_bloqueo_pantalla_teclado(pcb);
        printf("Valor a imprimir: %d\n",valor_a_imprimir);
        log_info(logger,BLU"Conexion Consola: Solicitud de imprimir valor."WHT);
        enviar_imprimir_valor(valor_a_imprimir,pcb->socket_consola);
    }
}

void desbloquear_pcb_screen(int socket){
    t_pcb* pcb = obtener_pcb_de_lista_segun_socket(socket,blocked_screen_list,mutex_blocked_screen);
    agregar_a_ready(pcb,BLOQUEAR_PROCESO_PANTALLA,BLOQUEADO_PANTALLA);
}

void* manejador_estado_blocked_keyboard(void* x){
    t_pcb* pcb;

    while(1){
        sem_wait(&bloquear_por_teclado);
        //pcb = quitar_pcb_de_queue(mutex_blocked_keyboard,blocked_keyboard_queue);
        pthread_mutex_lock(&mutex_blocked_keyboard);
        pcb = list_get(blocked_keyboard_list,list_size(blocked_keyboard_list)-1);
        pthread_mutex_unlock(&mutex_blocked_keyboard);

        pthread_mutex_lock(&mutex_logger);
        log_info(logger,string_from_format(CYN"PID: <%d> - Bloqueado por: <TECLADO>"WHT,pcb->pid));
        pthread_mutex_unlock(&mutex_logger);
        log_info(logger,BLU"Conexion Consola: Solicitud de valor."WHT);
        enviar_codigo_op(pcb->socket_consola,INPUT_VALOR);
    }
}

void desbloquear_pcb_keyboard(int socket, int input_consola){
    t_pcb* pcb = obtener_pcb_de_lista_segun_socket(socket,blocked_keyboard_list,mutex_blocked_keyboard);
    actualizar_registro_por_teclado(pcb,input_consola);
    agregar_a_ready(pcb,BLOQUEAR_PROCESO_TECLADO,BLOQUEADO_TECLADO);
}

t_pcb* obtener_pcb_de_lista_segun_socket(int socket,t_list* lista_pcb,pthread_mutex_t mutex){
    t_pcb* pcb;
    uint32_t indice_pcb = obtener_indice_pcb_segun_socket(socket,lista_pcb,mutex);
    pthread_mutex_lock(&mutex);
    pcb = list_remove(lista_pcb,indice_pcb);
    pthread_mutex_unlock(&mutex);
    return pcb;
}

int obtener_indice_pcb_segun_socket(int socket,t_list* lista_pcb,pthread_mutex_t mutex){

    uint32_t indice=0;
    t_pcb* pcb;

    pthread_mutex_lock(&mutex);
    uint32_t cantidad_elementos_lista = list_size(lista_pcb);

    for(uint32_t i=0; i < cantidad_elementos_lista; i++){
        pcb = list_get(lista_pcb,i);
        if(pcb->socket_consola == socket){
            indice=i;
        }
    }
    pthread_mutex_unlock(&mutex);
    return indice;
}


uint32_t obtener_tiempo_bloqueo(t_pcb* pcb){
    t_instruccion* instruccion_anterior = obtener_instruccion_anterior(pcb);
    dispositivo disp = obtener_dispositivo(pcb);
    return tiempos_bloqueos[disp] * instruccion_anterior->parametros[1] * 1000;
}

dispositivo obtener_dispositivo(t_pcb* pcb){
    t_instruccion* instruccion_anterior = obtener_instruccion_anterior(pcb);
    return (dispositivo) instruccion_anterior->parametros[0];
}

registro_cpu* obtener_registro_por_bloqueo_pantalla_teclado(t_pcb* pcb){
    t_instruccion* instruccion_anterior = obtener_instruccion_anterior(pcb);
    registro_cpu registro = instruccion_anterior->parametros[1];
    return obtener_registro(pcb, registro);
}

void actualizar_registro_por_teclado(t_pcb* pcb,uint32_t input){
    registro_cpu* registro_pcb = obtener_registro_por_bloqueo_pantalla_teclado(pcb);
    (*registro_pcb) = input;
    input=0;
}


t_instruccion* obtener_instruccion_anterior(t_pcb* pcb){
    t_instruccion* instruccion_anterior = list_get(pcb->lista_instrucciones,pcb->program_counter-1);
    return instruccion_anterior;
}

void agregar_a_ready(t_pcb* pcb, op_code motivo, estado_pcb anterior){
    if(algoritmo_es_feedback() && motivo == INTERRUPCION){
        logear_cambio_estado(pcb,anterior,READY2);
        agregar_pcb_a_lista(pcb,mutex_ready2,ready2_list);
    }else{
        logear_cambio_estado(pcb,anterior,READY1);
        agregar_pcb_a_lista(pcb,mutex_ready1,ready1_list);
    }

    printf(GRN"Cola de Ready1 <%s>: [ ",algoritmo_planificacion);
    for(uint32_t i=0; i < list_size(ready1_list); i++){
        uint32_t pid = ((t_pcb*) list_get(ready1_list,i))->pid;
        printf("<%d> ",pid);
    }
    printf("]\n"WHT);

    if(algoritmo_es_feedback()){
        printf(GRN"Cola de Ready2 <%s>: [ ",algoritmo_planificacion);
        for(uint32_t i=0; i < list_size(ready2_list); i++){
            uint32_t pid = ((t_pcb*) list_get(ready2_list,i))->pid;
            printf("<%d> ",pid);
        }
        printf("]\n"WHT);
    }

    sem_post(&pcbs_en_ready);
}

t_pcb* quitar_de_ready(estado_pcb* ready){
    if(!list_is_empty(ready1_list)){
        *ready = READY1;
        return quitar_primer_pcb_de_lista(mutex_ready1,ready1_list);
    }else{
        if(algoritmo_es_feedback() && !list_is_empty(ready2_list)){
            *ready = READY2;
            return quitar_primer_pcb_de_lista(mutex_ready2,ready2_list);
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