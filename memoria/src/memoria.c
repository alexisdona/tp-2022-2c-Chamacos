#include <sys/stat.h>
#include <fcntl.h>
#include "../headers/memoria.h"


void mostar_bitmap_frames();

int main(int argc, char* argv[]){

    validar_argumentos_main(argc);
    CONFIG_FILE = argv[1];
    logger = iniciar_logger(LOG_FILE, LOG_NAME);
    levantar_config();

    sem_init(&buscar_frame, 0, 0);
    pthread_mutex_init(&mutex_listas_frames_pendientes,NULL);
    pthread_mutex_init(&mutex_puntero_swap,NULL);

    lista_pid_frame = list_create();
    cola_pid_registro_tabla_paginas = queue_create();

    iniciar_estructuras_administrativas_kernel();
    crear_archivo_swap();
    //mostrar_contenido_archivo_swap();
    crear_espacio_usuario();
    frames_ocupados = dictionary_create();
    paginas_frames = queue_create();
    socket_kernel =  crear_conexion(ip_kernel, puerto_kernel);
    enviar_handshake_inicial(socket_kernel, MEMORIA, logger );

    pthread_create(&thread_escucha_kernel, NULL, conexion_kernel, (void*) (intptr_t) socket_kernel);
    pthread_detach(thread_escucha_kernel);
    
    pthread_create(&buscador_marcos, NULL, buscar_marcos_para_procesos, NULL);
    pthread_detach(buscador_marcos);

    socket_srv_memoria = levantar_servidor();
    log_info(logger, "Esperando CPU");
    while (socket_kernel != -1) {
        escuchar_cliente(socket_srv_memoria, logger);
    }

}

void levantar_config() {

    memoria_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();
    tamanio_memoria = config_get_int_value(memoria_config,"TAM_MEMORIA");
    tamanio_pagina = config_get_int_value(memoria_config,"TAM_PAGINA");
    entradas_por_tabla = config_get_int_value(memoria_config,"ENTRADAS_POR_TABLA");
    retardo_memoria = config_get_int_value(memoria_config,"RETARDO_MEMORIA");
    algoritmo_reemplazo = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO");
    tamanio_swap = config_get_int_value(memoria_config,"TAMANIO_SWAP");
    path_swap = config_get_string_value(memoria_config, "PATH_SWAP");
    ip_kernel = config_get_string_value(communication_config, "IP_KERNEL");
    puerto_kernel = config_get_int_value(communication_config, "PUERTO_KERNEL");
    ip_cpu = config_get_string_value(communication_config, "IP_CPU");
    puerto_cpu = config_get_int_value(communication_config, "PUERTO_CPU_DISPATCH");
    marcos_por_proceso = config_get_int_value(memoria_config, "MARCOS_POR_PROCESO");

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
    return iniciar_servidor(ip_memoria,puerto_memoria, logger);
}


void crear_bitmap_frames_libres() {
    uint32_t tamanio_bit_array = tamanio_memoria / tamanio_pagina;
    bloque_frames_libres = malloc(tamanio_bit_array);
    bitarray_frames = bitarray_create_with_mode(bloque_frames_libres, tamanio_bit_array, LSB_FIRST);
    inicializar_bitmap_frames();

}

void iniciar_estructuras_administrativas_kernel() {
    pthread_mutex_init(&mutex_tabla_paginas,NULL);
    crear_bitmap_frames_libres();
    registros_tabla_paginas = list_create();
    tabla_paginas = list_create();
}

void crear_espacio_usuario() {

    espacio_usuario_memoria = malloc(tamanio_memoria);
    uint32_t valor=0;
    for(int i=0; i< tamanio_memoria/sizeof(uint32_t); i++) {
        valor = i;
        memcpy(espacio_usuario_memoria + sizeof(uint32_t) *i , &valor, sizeof(uint32_t));
    }

    /*  printf("\nMEMORIA --> IMPRIMO VALORES EN ESPACIO DE USUARIO\n");
      for(int i=0; i< tamanio_memoria/sizeof(uint32_t); i++) {
          uint32_t* apuntado=  espacio_usuario_memoria + i*sizeof(uint32_t);
         printf("\nvalor apuntado en posición %d-->%d",i, *apuntado);
      }
      */
}

uint32_t crear_estructuras_administrativas_proceso(uint32_t tamanio_segmento, uint32_t pid, uint32_t contador_paginas ) {
    int cantidad_registros_tabla_segmentos = MAX(tamanio_segmento/tamanio_pagina, 1);
    for(int i=0; i < cantidad_registros_tabla_segmentos ; i++) {
        t_registro_tabla_paginas* registro_tabla_paginas = malloc(sizeof(t_registro_tabla_paginas));
        registro_tabla_paginas->numero_pagina = contador_paginas;
        registro_tabla_paginas->pid = pid;
        registro_tabla_paginas->frame = 0;
        registro_tabla_paginas->modificado = 0;
        registro_tabla_paginas->presencia = 0;
        registro_tabla_paginas->uso = 0;
        registro_tabla_paginas->posicion_swap = puntero_swap;
        actualizar_puntero_swap();
        list_add(registros_tabla_paginas, registro_tabla_paginas);
        contador_paginas++;
        log_info(logger, string_from_format("[indice:%d][registro:%d]registro_tabla_paginas->pid: %d", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->pid));
        log_info(logger, string_from_format("[indice:%d][registro:%d]registro_tabla_paginas->numero_pagina: %d", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->numero_pagina));

        /* log_info(logger,string_from_format("[indice:%d][registro:%d]registro_tabla_paginas->frame: %d", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->frame));
         log_info(logger,string_from_format("[indice:%d][registro:%d]registro_tabla_paginas->modificado: %d", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->modificado));
         log_info(logger,string_from_format("[indice:%d][registro:%d]registro_tabla_paginas->presencia: %d", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->presencia));
         log_info(logger,string_from_format("[indice:%d][registro:%d]registro_tabla_paginas->uso: %d", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->uso));
         log_info(logger,string_from_format("[indice:%d][registro:%d]registro_tabla_paginas->posicion_swap: %d\n", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->posicion_swap));
 */
    }
    pthread_mutex_lock(&mutex_tabla_paginas);
    list_add(tabla_paginas, list_duplicate(registros_tabla_paginas));
    pthread_mutex_unlock(&mutex_tabla_paginas);
    list_clean(registros_tabla_paginas);
    return list_size(tabla_paginas);
}

void crear_archivo_swap() {
    void *str = malloc(tamanio_swap);
    uint32_t cantidad_bloques = tamanio_swap/sizeof(uint32_t);
    uint32_t valor = 0;
    // lleno las páginas del proceso con enteros partiendo desde id_proceso
    for(int i=0; i< cantidad_bloques ; i++) {
        memcpy(str + (sizeof(uint32_t) *i) , &valor, sizeof(uint32_t));
    }

/*
    printf("\nswap --> IMPRIMO VALORES EN el archivo\n");
      for(int i=0; i< cantidad_bloques; i++) {
          uint32_t* apuntado=  str+ sizeof(uint32_t) *i;
         printf("\nvalor apuntado en posición del arhivo %d-->%d\n",i, *apuntado);
   }
*/

    FILE *archivo_swap = fopen(path_swap, "wb");
    if (archivo_swap != NULL) {
        fwrite(str, sizeof (uint32_t), cantidad_bloques, archivo_swap);
    }
    else {
        perror("Error abriendo el archivo swap: ");
    }

    fclose(archivo_swap);
    free(str);
}

void escuchar_cliente(int socket_server, t_log* logger) {
    socket_cpu = esperar_cliente(socket_server, logger);
    void* retorno;
    if (socket_cpu != -1) {
        pthread_t hilo;
        pthread_create(&hilo, NULL, (void*) procesar_conexion, NULL);
        pthread_join(hilo,&retorno);
    }

}

void procesar_conexion(void* args) {
    printf("se conecta un cliente.\n");
    handshake_cpu_memoria(socket_cpu, tamanio_pagina, entradas_por_tabla);

    while(socket_cpu != -1) {
        op_code cod_op = recibir_operacion(socket_cpu);

        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(socket_cpu, logger);
                break;
            case ESCRIBIR_MEMORIA:
                ;
                uint32_t desplazamiento = 0; // es el deplazamiento para moverme en el buffer que recibo
                uint32_t id_proceso_escribir_memoria;
                uint32_t numero_pagina_escribir_memoria; //este numero es relativo al segmento
                uint32_t marco_escribir_memoria;

                uint32_t desplazamiento_escribir_memoria;
                uint32_t indice_tabla_paginas_escribir_memoria;
                uint32_t valor_escribir_memoria;
                void* buffer_escribir_memoria = recibir_buffer(socket_cpu);

                memcpy(&id_proceso_escribir_memoria, buffer_escribir_memoria+desplazamiento, sizeof (uint32_t));
                desplazamiento += sizeof (uint32_t);
                memcpy(&numero_pagina_escribir_memoria, buffer_escribir_memoria+desplazamiento, sizeof (uint32_t));
                desplazamiento += sizeof (uint32_t);
                memcpy(&marco_escribir_memoria, buffer_escribir_memoria+desplazamiento, sizeof (uint32_t));
                desplazamiento += sizeof (uint32_t);
                memcpy(&desplazamiento_escribir_memoria, buffer_escribir_memoria+desplazamiento, sizeof (uint32_t));
                desplazamiento += sizeof (uint32_t);
                memcpy(&indice_tabla_paginas_escribir_memoria, buffer_escribir_memoria+desplazamiento, sizeof (uint32_t));
                desplazamiento += sizeof (uint32_t);
                memcpy(&valor_escribir_memoria, buffer_escribir_memoria+desplazamiento, sizeof (uint32_t));
                desplazamiento += sizeof (uint32_t);
                memcpy((espacio_usuario_memoria +(marco_escribir_memoria*tamanio_pagina+desplazamiento_escribir_memoria)), &valor_escribir_memoria, sizeof(uint32_t));

                enviar_mensaje(string_from_format("Escribi el valor: %d en el marco: %d desplazamiento :%d", valor_escribir_memoria, marco_escribir_memoria, desplazamiento_escribir_memoria), socket_cpu);

                t_registro_tabla_paginas * registro_escrito_memoria = obtener_registro_tabla_paginas(indice_tabla_paginas_escribir_memoria, numero_pagina_escribir_memoria);
                actualizar_bit_modificado(registro_escrito_memoria);

                break;
            case LEER_MEMORIA:
                ;
                uint32_t id_proceso_leer_memoria;
                uint32_t indica_pagina_leer_memoria;
                uint32_t numero_pagina_leer_memoria; //este numero es relativo al segmento
                uint32_t marco_leer_memoria;
                uint32_t desplazamiento_leer_memoria;
                int despl=0;
                void* buffer_leer_memoria = recibir_buffer(socket_cpu);
                memcpy(&id_proceso_leer_memoria, buffer_leer_memoria+despl, sizeof (uint32_t));
                despl+=sizeof(uint32_t);
                memcpy(&indica_pagina_leer_memoria, buffer_leer_memoria+despl, sizeof (uint32_t));
                despl+=sizeof(uint32_t);
                memcpy(&numero_pagina_leer_memoria, buffer_leer_memoria+despl, sizeof (uint32_t));
                despl+=sizeof(uint32_t);
                memcpy(&marco_leer_memoria, buffer_leer_memoria+despl, sizeof (uint32_t));
                despl+=sizeof(uint32_t);
                memcpy(&desplazamiento_leer_memoria, buffer_leer_memoria+despl, sizeof (uint32_t));
                uint32_t* valor_leido = (uint32_t *) (espacio_usuario_memoria +(marco_leer_memoria*tamanio_pagina+desplazamiento_leer_memoria));
                enviar_entero(socket_cpu, *valor_leido, LEER_MEMORIA);
                t_registro_tabla_paginas * registro_leido_memoria = obtener_registro_tabla_paginas(indice_tabla_paginas_escribir_memoria, numero_pagina_escribir_memoria);
                actualizar_bit_uso(registro_leido_memoria);
                break;
            case OBTENER_MARCO:
                ;
                void* buffer_marco = recibir_buffer(socket_cpu);
                usleep(retardo_memoria*1000);
                uint32_t id_proceso_marco;
                uint32_t nro_tabla_obtener_marco;
                uint32_t numero_pag_obtener_marco;
                int desp = 0;
                int marco = -1;
                memcpy(&id_proceso_marco, buffer_marco+desp, sizeof(uint32_t));
                desp+= sizeof(uint32_t);

                memcpy(&nro_tabla_obtener_marco, buffer_marco+desp, sizeof(uint32_t));
                desp+= sizeof(uint32_t);

                memcpy(&numero_pag_obtener_marco, buffer_marco + desp, sizeof(uint32_t));
                t_registro_tabla_paginas* registro_tabla_paginas = obtener_registro_tabla_paginas(nro_tabla_obtener_marco, numero_pag_obtener_marco);

                if(registro_tabla_paginas->presencia) {
                    log_info(logger,"Presencia");
                    marco = registro_tabla_paginas->frame;
                    enviar_marco(socket_cpu, marco);
                } else {
                    log_info(logger,"MEMORIA --> Enviando page_fault");
                    actualizar_lista_frames_pendientes(registro_tabla_paginas);
                    sem_post(&buscar_frame);
                    enviar_codigo_op(socket_cpu,PAGE_FAULT);
                }
                break;
            case -1:
                log_info(logger, "El cliente se desconectó");
                socket_cpu = -1;
                break;
            default:
                printf(RED"codigo operacion %d"RESET,cod_op);
                log_warning(logger, "Operacion desconocida.");
                break;
        }
    }
}

t_registro_tabla_paginas *obtener_registro_tabla_paginas(uint32_t indice_tabla_paginas,
                                                         uint32_t numero_pagina) {
    return (t_registro_tabla_paginas *) (list_get(list_get(tabla_paginas, indice_tabla_paginas), numero_pagina)); }

void actualizar_bit_modificado(t_registro_tabla_paginas* registro_tabla_paginas) {
    registro_tabla_paginas->modificado = 1;
    actualizar_bit_uso(registro_tabla_paginas);
}

void actualizar_bit_uso(t_registro_tabla_paginas* registro_tabla_paginas) {
    registro_tabla_paginas->uso = 1;
}

void enviar_marco(int cliente_fd, int marco) {
    enviar_entero8bytes(cliente_fd, marco, OBTENER_MARCO);
}

void buscar_frame_libre_proceso(t_registro_tabla_paginas *registro_tabla_paginas) {
    uint32_t cantidad_marcos_ocupados_proceso = obtener_cantidad_marcos_ocupados_proceso(registro_tabla_paginas->pid);
    if (marcos_por_proceso > cantidad_marcos_ocupados_proceso) {
        int frame_libre = obtener_numero_frame_libre();
        void* pagina_swap = obtener_bloque_proceso_desde_swap(registro_tabla_paginas->posicion_swap);
        memcpy(espacio_usuario_memoria + (frame_libre * tamanio_pagina), pagina_swap, tamanio_pagina);
        registro_tabla_paginas->frame = (uint32_t) frame_libre;
        registro_tabla_paginas->presencia = 1;
        registro_tabla_paginas->uso = 1;
        agregar_a_cola_frames_por_paginas(registro_tabla_paginas);
    }
    else {
        if (string_equals_ignore_case(algoritmo_reemplazo, "CLOCK")) {
            //ejecuta clock
            log_info(logger, "tiene que ejecutar clock");
            ejecutar_clock(registro_tabla_paginas);
        }
        else {
           // ejecuta clock modificado
            log_info(logger, "tiene que ejecutar clock modificado");
            ejecutar_clock_modificado(registro_tabla_paginas);
        }
        //ejecutar algoritmo de reemplazo
    }
    enviar_codigo_op(socket_kernel, PAGE_FAULT_ATENDIDO);
}


void *conexion_kernel(void* socket){
    socket_kernel = (intptr_t) socket;
    log_info(logger, "socket_kernel: %d", socket_kernel);
    while(socket_kernel != -1) {
        op_code codigo_operacion = recibir_operacion(socket_kernel);
        switch(codigo_operacion) {
            case CREAR_ESTRUCTURAS_ADMIN:
                ;
                t_pcb* pcb = recibir_PCB(socket_kernel);
                log_info(logger, "Creando estructuras administrativas");
                uint32_t contador_paginas =0;
                pthread_mutex_lock(&liberar_estructuras);
                for(int i=0; i < list_size(pcb->tabla_segmentos); i++){
                    t_segmento * segmento;
                    segmento = list_get(pcb->tabla_segmentos, i);
                    segmento->indice_tabla_paginas = (crear_estructuras_administrativas_proceso(segmento->tamanio_segmento, pcb->pid, contador_paginas)-1);
                    contador_paginas = contador_paginas + (segmento->tamanio_segmento / tamanio_pagina);
                }
                pthread_mutex_unlock(&liberar_estructuras);
                enviar_PCB(socket_kernel, pcb, ACTUALIZAR_INDICE_TABLA_PAGINAS);
                break;

            case FINALIZAR_PROCESO:
                ;
                log_info(logger, "entra en finalizar proceso");
                uint32_t pid_finalizado = recibir_valor(socket_kernel);
                log_info(logger, "Liberando memoria del proceso");
                pthread_mutex_lock(&liberar_estructuras);
                liberar_tablas_paginas_proceso(pid_finalizado);
                pthread_mutex_unlock(&liberar_estructuras);
                break;
            default:
                break;
        }
    }
}

void handshake_cpu_memoria(int socket_destino, uint32_t tamanio_pagina, uint32_t cantidad_entradas_tabla) {
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = HANDSHAKE_CPU_MEMORIA;

    agregar_entero(paquete, tamanio_pagina);
    agregar_entero(paquete, cantidad_entradas_tabla);
    enviar_paquete(paquete, socket_destino);
    eliminar_paquete(paquete);
}

void actualizar_puntero_swap(){
    pthread_mutex_lock(&mutex_puntero_swap);
    puntero_swap+=tamanio_pagina;
    pthread_mutex_unlock(&mutex_puntero_swap);

}

int obtener_numero_frame_libre() {

    for(int i= 0; bitarray_frames->size; i++) {
        if (bitarray_test_bit(bitarray_frames, i) == 0) {
            bitarray_set_bit(bitarray_frames, i);
            return i;
        }
    }
    return -1;
}

void* obtener_bloque_proceso_desde_swap(uint32_t posicion_swap) {

    FILE* archivo_swap = fopen(path_swap, "rb");
    void* pagina_swap = malloc(tamanio_pagina);
    fseek(archivo_swap,posicion_swap,SEEK_SET);
    fread(pagina_swap, tamanio_pagina, 1, archivo_swap);
    fclose(archivo_swap);
/*
    for(int i = 0; i<tamanio_pagina/4; i++) {
        printf("pagina[%lu]:%lu", i*sizeof(uint32_t), (uint32_t*) (pagina_swap + sizeof(uint32_t)*i));
    }
*/
    return pagina_swap; //devuelve la pagina entera que es del tamano de pagina
}

uint32_t obtener_cantidad_marcos_ocupados_proceso(uint32_t id_proceso_marco) {

    uint32_t cantidad_marcos_ocupados = 0;
    for ( int i = 0; i < list_size(tabla_paginas); i++) {
        t_list *registros = list_get(tabla_paginas, i);
        for (int j = 0; j < list_size(registros); j++) {
            t_registro_tabla_paginas *registro = list_get(registros, j);
            if (registro->pid == id_proceso_marco && registro->presencia == 1) {
                cantidad_marcos_ocupados++;
            }
        }
    }
    return cantidad_marcos_ocupados;
}

void* buscar_marcos_para_procesos(void* args){

    t_registro_tabla_paginas* registro_tabla_paginas = malloc(sizeof(t_registro_tabla_paginas));

    while(1){
        sem_wait(&buscar_frame);
        registro_tabla_paginas = obtener_tupla_elementos_lista_frames_pendientes(registro_tabla_paginas);
        buscar_frame_libre_proceso(registro_tabla_paginas);
    }
}

void actualizar_lista_frames_pendientes(t_registro_tabla_paginas* registro){
    pthread_mutex_lock(&mutex_listas_frames_pendientes);
    queue_push(cola_pid_registro_tabla_paginas, registro);
    pthread_mutex_unlock(&mutex_listas_frames_pendientes);
}

t_registro_tabla_paginas *obtener_tupla_elementos_lista_frames_pendientes(){
    pthread_mutex_lock(&mutex_listas_frames_pendientes);
    t_registro_tabla_paginas * registro;
    registro = (t_registro_tabla_paginas *) queue_pop(cola_pid_registro_tabla_paginas);
    pthread_mutex_unlock(&mutex_listas_frames_pendientes);
    return registro;
}

void agregar_a_cola_frames_por_paginas(t_registro_tabla_paginas* registro_tabla_paginas) {

    t_queue* cola_frames_paginas = dictionary_get(frames_ocupados, string_itoa(registro_tabla_paginas->pid));
    if (cola_frames_paginas != NULL) {
        queue_push(cola_frames_paginas, registro_tabla_paginas);
    }
    else {
        t_queue * cola = queue_create();
        queue_push(cola, registro_tabla_paginas);
        dictionary_put(frames_ocupados, string_itoa(registro_tabla_paginas->pid), cola);
    }
}

void ejecutar_clock(t_registro_tabla_paginas* registro_tabla_paginas_nuevo){
    t_queue* paginas_frames_proceso =  dictionary_get(frames_ocupados, string_itoa((int) registro_tabla_paginas_nuevo->pid));
    uint hay_pagina_victima = 0;

    while(hay_pagina_victima == 0) {
        t_registro_tabla_paginas* registro = queue_pop(paginas_frames_proceso);
        if (registro->uso == 0) {
            if (registro->modificado == 1) {
                actualizar_pagina_en_swap(registro);
            }
            registro->presencia = 0;
            registro_tabla_paginas_nuevo->presencia = 1;
            registro_tabla_paginas_nuevo->uso = 1;
            registro_tabla_paginas_nuevo->frame = registro->frame;
            queue_push(paginas_frames_proceso, registro_tabla_paginas_nuevo);
            hay_pagina_victima = 1;
        } else {
            registro->uso = 0;
            queue_push(paginas_frames_proceso, registro);
        }
    }

}

void ejecutar_clock_modificado(t_registro_tabla_paginas* registro_tabla_paginas_nuevo ){

    t_queue* paginas_frames_proceso =  dictionary_get(frames_ocupados, string_itoa((int) registro_tabla_paginas_nuevo->pid));
    uint hay_pagina_victima = 0;
    uint32_t vuelta_algoritmo = 0;
    t_registro_tabla_paginas* registro;
    while(hay_pagina_victima == 0) {
         registro = queue_pop(paginas_frames_proceso);
        // busco (0,0)
        if (registro->uso == 0 && registro->modificado == 0) {
            registro->presencia = 0;
            registro_tabla_paginas_nuevo->presencia = 1;
            registro_tabla_paginas_nuevo->uso = 1;
            registro_tabla_paginas_nuevo->frame = registro->frame;
            queue_push(paginas_frames_proceso, registro_tabla_paginas_nuevo);
            hay_pagina_victima = 1;
        } else {
            queue_push(paginas_frames_proceso, registro);
            vuelta_algoritmo+=1;
            if (vuelta_algoritmo >= marcos_por_proceso) {
                //busco (0,1)
                registro = queue_pop(paginas_frames_proceso);
                if (registro->uso == 0 && registro->modificado == 1) {
                    actualizar_pagina_en_swap(registro);
                    registro->presencia = 0;
                    registro_tabla_paginas_nuevo->presencia = 1;
                    registro_tabla_paginas_nuevo->uso = 1;
                    registro_tabla_paginas_nuevo->frame = registro->frame;
                    //actualizar swap si el bit de moficiado del registro victima estaba en 1
                    queue_push(paginas_frames_proceso, registro_tabla_paginas_nuevo);
                    hay_pagina_victima = 1;
                }
                else {
                    registro->uso = 0;
                    queue_push(paginas_frames_proceso, registro);
                }
            }
        }

    }
}

void actualizar_pagina_en_swap(t_registro_tabla_paginas* registro) {
  //  mostrar_contenido_swap(registro->posicion_swap);
    void* pagina = malloc(tamanio_pagina);
    memcpy(pagina, espacio_usuario_memoria+(registro->frame*tamanio_pagina), tamanio_pagina);
    FILE *archivo_swap = fopen(path_swap, "wb");
    fseek(archivo_swap, registro->posicion_swap, SEEK_SET);
    if (archivo_swap != NULL) {
        fwrite(pagina, tamanio_pagina, 1, archivo_swap);
    }
    else {
        perror("Error abriendo el archivo swap: ");
    }
    fclose(archivo_swap);
  //  mostrar_contenido_swap(registro->posicion_swap);
    free(pagina);
}
/*******
 * FUNCIONES AUXILIARES
 */

void mostrar_contenido_swap(uint32_t puntero_desde) {

    void* pagina = malloc(tamanio_pagina);
    FILE *archivo_swap = fopen(path_swap, "rb");
    fseek(archivo_swap, puntero_desde, SEEK_SET);
    if (archivo_swap != NULL) {
        fread(pagina, tamanio_pagina, 1, archivo_swap);
    }
    else {
        perror("Error abriendo el archivo swap: ");
    }


    for(int i=0; i< tamanio_pagina; i++) {
        uint32_t *apuntado = pagina + sizeof(uint32_t) * i;
        printf("\nvalor[%d]-->%d", i, *apuntado);
    }

    fclose(archivo_swap);
    free(pagina);
}

void liberar_tablas_paginas_proceso(uint32_t pid) {
  //  mostar_bitmap_frames();
    for (int i=0; i<list_size(tabla_paginas); i++) {
        t_list* registros_tabla_paginas = list_get(tabla_paginas, i);
        for (int j=0; j< list_size(registros_tabla_paginas); j++) {
            t_registro_tabla_paginas* registro = (t_registro_tabla_paginas *) list_get(registros_tabla_paginas, j);
            if (registro->pid == pid) {
                registro->presencia = 0;
                bitarray_clean_bit(bitarray_frames, registro->frame);
            }
        }
    }

   // mostar_bitmap_frames();
}

void mostar_bitmap_frames() {
    log_info(logger, "valores del bitarray despues de liberar memoria");
    for(int i= 0; i<(tamanio_memoria/tamanio_pagina); i++) {
        log_info(logger, string_from_format("bitarray_frames[%d]:%d", i, bitarray_test_bit(bitarray_frames, i)));
    }
}


void inicializar_bitmap_frames() {
    for(int i= 0; i<(tamanio_memoria/tamanio_pagina); i++) {
        bitarray_clean_bit(bitarray_frames, i);
    }
}

