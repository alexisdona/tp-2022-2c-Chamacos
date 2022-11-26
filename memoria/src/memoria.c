#include <sys/stat.h>
#include <fcntl.h>
#include "../headers/memoria.h"



int main(int argc, char* argv[]){

    validar_argumentos_main(argc);
    CONFIG_FILE = argv[1];
    logger = iniciar_logger(LOG_FILE, LOG_NAME);
    levantar_config();
    iniciar_estructuras_administrativas_kernel();
    crear_archivo_swap();
    //mostrar_contenido_archivo_swap();
    crear_espacio_usuario();

    socket_kernel =  crear_conexion(ip_kernel, puerto_kernel);
    enviar_handshake_inicial(socket_kernel, MEMORIA, logger );
    pthread_create(&thread_escucha_kernel, NULL, conexion_kernel, (void*) (intptr_t) socket_kernel);
    pthread_detach(thread_escucha_kernel);

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
    frames_disponibles = bitarray_create_with_mode(bloque_frames_libres, tamanio_bit_array, LSB_FIRST);
}

void iniciar_estructuras_administrativas_kernel() {
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

uint32_t crear_estructuras_administrativas_proceso(uint32_t tamanio_segmento, uint32_t pid ) {
    int cantidad_registros_tabla_segmentos = MAX(tamanio_segmento/tamanio_pagina, 1);
    t_registro_tabla_paginas* registro_tabla_paginas = malloc(sizeof(t_registro_tabla_paginas));
    for(int i=0; i < cantidad_registros_tabla_segmentos ; i++) {
        registro_tabla_paginas->pid = pid;
        registro_tabla_paginas->frame = 0;
        registro_tabla_paginas->modificado = 0;
        registro_tabla_paginas->presencia = 0;
        registro_tabla_paginas->uso = 0;
        registro_tabla_paginas->posicion_swap = puntero_swap;
        actualizar_puntero_swap();
        list_add(registros_tabla_paginas, registro_tabla_paginas);
    /*    printf("\n[indice:%d][registro:%d]registro_tabla_paginas->pid: %d", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->pid);
        printf("\n[indice:%d][registro:%d]registro_tabla_paginas->frame: %d", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->frame);
        printf("\n[indice:%d][registro:%d]registro_tabla_paginas->modificado: %d", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->modificado);
        printf("\n[indice:%d][registro:%d]registro_tabla_paginas->presencia: %d", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->presencia);
        printf("\n[indice:%d][registro:%d]registro_tabla_paginas->uso: %d", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->uso);
        printf("\n[indice:%d][registro:%d]registro_tabla_paginas->posicion_swap: %d\n", list_size(tabla_paginas), list_size(registros_tabla_paginas), registro_tabla_paginas->posicion_swap);
   */
    }
    list_add(tabla_paginas, list_duplicate(registros_tabla_paginas));
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
        valor+=1;
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
}

void mostrar_contenido_archivo_swap() {
    int archivo_swap = open(path_swap, O_RDWR);
    struct stat sb;
    if (fstat(archivo_swap,&sb) == -1) {
        perror("No se pudo obtener el size del archivo swap: ");
    }

    if (archivo_swap != NULL) {
        void* lectura = malloc(sb.st_size);
        read(archivo_swap, lectura, sb.st_size );
        printf(YEL"\n\nEL CONTENIDO DEL ARCHIVO POST SWAPEO\n\n"RESET);
          for(int i=0; i< sb.st_size / sizeof (uint32_t); i++) {
              uint32_t* apuntado=  lectura+ sizeof(uint32_t) *i;
              printf("\nvalor apuntado en posición del arhivo actualizado%d-->%d\n",i, *apuntado);
          }
    }
}

void escuchar_cliente(int socket_server, t_log* logger) {
    int cliente = esperar_cliente(socket_server, logger);
    if (cliente != -1) {
        pthread_t hilo;
        t_procesar_conexion_attrs* attrs = malloc(sizeof(t_procesar_conexion_attrs));
        attrs->log = logger;
        attrs->fd = cliente;
        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) attrs);
        pthread_detach(hilo);
    }

}

void procesar_conexion(void* void_args) {
    printf("se conecta un cliente.\n");
    t_procesar_conexion_attrs* attrs = (t_procesar_conexion_attrs*) void_args;
    t_log* logger = attrs->log;
    int cliente_fd = attrs->fd;
    free(attrs);
    handshake_cpu_memoria(cliente_fd, tamanio_pagina, entradas_por_tabla);

    while(cliente_fd != -1) {
        op_code cod_op = recibir_operacion(cliente_fd);
        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(cliente_fd, logger);
                break;
            case ESCRIBIR_MEMORIA:
                break;
            case LEER_MEMORIA:
                ;
                uint32_t marco_leer_memoria;
                uint32_t desplazamiento_leer_memoria;
                void* buffer_marco_leer_memoria = recibir_buffer(cliente_fd);
                memcpy(&marco_leer_memoria, buffer_marco_leer_memoria, sizeof (uint32_t));
                memcpy(&desplazamiento_leer_memoria, buffer_marco_leer_memoria + sizeof (uint32_t), sizeof(uint32_t));
                printf("Me pidieron leer de memoria en el marco: %d", marco_leer_memoria);
                uint32_t* valor_leido = (uint32_t *) (espacio_usuario_memoria +(marco_leer_memoria*tamanio_pagina+desplazamiento_leer_memoria));
                enviar_entero(cliente_fd, *valor_leido, LEER_MEMORIA);
                //falta actualizar el bit de uso en 1 en la tabla de paginas. Hay que recibir el indice de la tabla para saber cual es.
                break;
            case OBTENER_MARCO:
                printf(GRN"\n");
                uint32_t cantidad_marcos_ocupados_proceso=0;
                usleep(retardo_memoria*1000);
                void* buffer_marco = recibir_buffer(cliente_fd);
                uint32_t id_proceso_marco;
                uint32_t nro_tabla_obtener_marco;
                uint32_t numero_pag_obtener_marco;
                int despl = 0;
                int marco;
                memcpy(&id_proceso_marco, buffer_marco+despl, sizeof(uint32_t));
                despl+= sizeof(uint32_t);
                memcpy(&nro_tabla_obtener_marco, buffer_marco+despl, sizeof(uint32_t));
                despl+= sizeof(uint32_t);
                memcpy(&numero_pag_obtener_marco, buffer_marco + despl, sizeof(uint32_t));
                t_registro_tabla_paginas * registro_tabla_paginas = (t_registro_tabla_paginas *) (list_get(list_get(tabla_paginas, nro_tabla_obtener_marco), numero_pag_obtener_marco));

                if(registro_tabla_paginas->presencia) {
                    marco = registro_tabla_paginas->frame;
                    enviar_marco(cliente_fd, marco);
                } else {
                    cod_op = PAGE_FAULT;
                    printf("MEMORIA --> Enviando page_fault");
                    enviar_page_fault_cpu(cliente_fd, cod_op, marco);
                    buscar_frame_libre_proceso(id_proceso_marco, registro_tabla_paginas);
                }
                break;
            case TERMINAR_PROCESO:
                break;
            case -1:
                log_info(logger, "El cliente se desconectó");
                cliente_fd = -1;
                break;
            default:
                printf(RED"codigo operacion %d"RESET,cod_op);
                log_warning(logger, "Operacion desconocida.");
                break;
        }
    }
}

void enviar_marco(int cliente_fd, int marco) {
    enviar_entero8bytes(cliente_fd, marco, OBTENER_MARCO);
}

void buscar_frame_libre_proceso(uint32_t id_proceso_marco, t_registro_tabla_paginas *registro_tabla_paginas) {
    int frame_libre = obtener_numero_frame_libre();
    uint32_t cantidad_marcos_ocupados_proceso = obtener_cantidad_marcos_ocupados_proceso(id_proceso_marco);
    if (marcos_por_proceso > cantidad_marcos_ocupados_proceso) {
        void* pagina_swap = obtener_bloque_proceso_desde_swap(registro_tabla_paginas->posicion_swap);
        memcpy(espacio_usuario_memoria + (frame_libre * tamanio_pagina), pagina_swap, tamanio_pagina);
        registro_tabla_paginas->frame = (uint32_t) frame_libre;
        registro_tabla_paginas->presencia = 1;
        registro_tabla_paginas->uso = 1;
    }
}

void enviar_page_fault_cpu(int cliente_fd, op_code cod_op, int marco) {
    enviar_entero8bytes(cliente_fd, marco, cod_op);
}


void *conexion_kernel(void* socket){
    socket_kernel = (intptr_t) socket;
    log_info(logger, "socket_kernel: %d", socket_kernel);
    while(socket_kernel != -1) {
        op_code codigo_operacion = recibir_operacion(socket_kernel);
        switch(codigo_operacion) {
            case CREAR_ESTRUCTURAS_ADMIN:
                ;
                log_info(logger, "creando estructuras administrativas");
                t_pcb* pcb = recibir_PCB(socket_kernel);
                for(int i=0; i < list_size(pcb->tabla_segmentos); i++){
                    t_segmento * segmento = malloc(sizeof(t_segmento));
                    segmento = list_get(pcb->tabla_segmentos, i);
                    segmento->indice_tabla_paginas = (crear_estructuras_administrativas_proceso(segmento->tamanio_segmento, pcb->pid)-1);
               //     log_info(logger, "segmento[%d]->tamaño: %d",i, segmento->tamanio_segmento);
               //     log_info(logger, "segmento[%d]->indice_tabla: %d", i, segmento->indice_tabla_paginas );
                }
                enviar_PCB(socket_kernel, pcb, ACTUALIZAR_INDICE_TABLA_PAGINAS);
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
    //falta un mutex que encierre esta seccion critica
    puntero_swap+=tamanio_pagina;
}

int obtener_numero_frame_libre() {

    for(int i= 0; frames_disponibles->size; i++) {
        if ( bitarray_test_bit(frames_disponibles, i) == 0) {
            bitarray_set_bit(frames_disponibles, i);
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

