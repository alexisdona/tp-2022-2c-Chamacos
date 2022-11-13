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
    bloque_frames_lilbres = malloc(tamanio_bit_array);
    frames_disponibles = bitarray_create_with_mode(bloque_frames_lilbres, tamanio_bit_array, LSB_FIRST);
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

uint32_t crear_estructuras_administrativas_proceso(uint32_t tamanio_segmento ) {
    int cantidad_registros_tabla_segmentos = tamanio_segmento/tamanio_pagina;
    t_registro_tabla_paginas* registro_tabla_paginas = malloc(sizeof(t_registro_tabla_paginas));

    for(int i=0; i<cantidad_registros_tabla_segmentos; i++) {
        registro_tabla_paginas->numero_pagina=i;
        registro_tabla_paginas->frame = 0;
        registro_tabla_paginas->modificado = 0;
        registro_tabla_paginas->presencia = 0;
        registro_tabla_paginas->uso = 0;
        registro_tabla_paginas->posicion_swap = obtener_ultima_posicion_swap();
        list_add(registros_tabla_paginas, registro_tabla_paginas);
    }
    list_add(tabla_paginas, list_duplicate(registros_tabla_paginas));
    list_clean(registros_tabla_paginas);
    free(registro_tabla_paginas);
    return list_size(tabla_paginas);
}

uint32_t obtener_ultima_posicion_swap() {
    uint32_t ultima_posicion_swap = 64;
    return ultima_posicion_swap;
}

void crear_archivo_swap() {
    void *str = malloc(tamanio_swap);
    uint32_t cantidad_bloques = tamanio_swap/tamanio_pagina;
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

    while(cliente_fd != -1) {
        handshake_cpu_memoria(cliente_fd, tamanio_pagina, entradas_por_tabla);
        op_code cod_op = recibir_operacion(cliente_fd);
        log_info(logger, "EN MEMORIA RECIBI un cod_op %s", cod_op);
        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(cliente_fd, logger);
                break;
            case ESCRIBIR_MEMORIA:
                break;
            case LEER_MEMORIA:
                break;

            case OBTENER_MARCO:
                break;
            case TERMINAR_PROCESO:
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


void *conexion_kernel(void* socket){
    socket_kernel = (intptr_t) socket;
    log_info(logger, "socket_kernel: %d", socket_kernel);
    while(socket_kernel != -1) {
        op_code codigo_operacion = recibir_operacion(socket_kernel);
        switch(codigo_operacion) {
            case CREAR_ESTRUCTURAS_ADMIN:
                ;
                t_pcb* pcb = recibir_PCB(socket_kernel);
                log_info(logger, "creando estructuras administrativas");

                for(int i=0; i < list_size(pcb->tabla_segmentos); i++){
                    t_segmento * segmento = malloc(sizeof(t_segmento));
                    segmento = list_get(pcb->tabla_segmentos, i);
                    segmento->indice_tabla_paginas = (crear_estructuras_administrativas_proceso(segmento->tamanio_segmento)-1);

                }
                enviar_PCB(socket_kernel, pcb, ACTUALIZAR_INDICE_TABLA_PAGINAS);
                break;

        }
    }
}


void *conexion_cpu(void* socket){
    socket_cpu = (intptr_t) socket;
    log_info(logger, "socket_cpu: %d", socket_cpu);
    while(socket_cpu != -1) {
        op_code codigo_operacion = recibir_operacion(socket_cpu);
        switch(codigo_operacion) {

        }
    }
    return EXIT_SUCCESS;
}

void handshake_cpu_memoria(int socket_destino, uint32_t tamanio_pagina, uint32_t cantidad_entradas_tabla) {
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = HANDSHAKE_CPU_MEMORIA;

    agregar_entero(paquete, tamanio_pagina);
    agregar_entero(paquete, cantidad_entradas_tabla);
    enviar_paquete(paquete, socket_destino);
    eliminar_paquete(paquete);
}
