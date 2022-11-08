#include <sys/stat.h>
#include <fcntl.h>
#include "../headers/memoria.h"

int main(int argc, char* argv[]){

    validar_argumentos_main(argc);
	
	char* CONFIG_FILE = argv[1];

    logger = iniciar_logger(LOG_FILE, LOG_NAME);
    memoria_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();

	//Levanto el servidor de memoria
    int socket_srv_memoria = levantar_servidor();
    //Espero la conexion de la cpu
    log_info(logger,"Esperando CPU..");


	char* IP_KERNEL = config_get_string_value(communication_config,"IP_KERNEL");
	int PUERTO_KERNEL = config_get_int_value(communication_config,"PUERTO_KERNEL");

    tamanio_memoria = config_get_int_value(memoria_config,"TAM_MEMORIA");
    tamanio_pagina = config_get_int_value(memoria_config,"TAM_PAGINA");
    entradas_por_tabla = config_get_int_value(memoria_config,"ENTRADAS_POR_TABLA");
    retardo_memoria = config_get_int_value(memoria_config,"RETARDO_MEMORIA");
    algoritmo_reemplazo = config_get_string_value(memoria_config, "ALGORITMO_REEMPLAZO");
    tamanio_swap = config_get_int_value(memoria_config,"TAMANIO_SWAP");
    path_swap = config_get_string_value(memoria_config, "PATH_SWAP");
    iniciar_estructuras_administrativas_kernel();
    crear_archivo_swap();
    //mostrar_contenido_archivo_swap();
    crear_espacio_usuario();
	int socket_kernel = crear_conexion(IP_KERNEL,PUERTO_KERNEL);
    int socket_cliente = esperar_cliente(socket_srv_memoria,logger);
    uint32_t modulo = recibir_handshake_inicial(socket_cliente,MEMORIA,logger);
    uint32_t respuesta = enviar_handshake_inicial(socket_kernel,MEMORIA,logger);
    while(socket_kernel != -1){
        op_code codigo_operacion = recibir_operacion(socket_kernel);
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
    char* ip_memoria = config_get_string_value(communication_config,"IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(communication_config,"PUERTO_MEMORIA");
    return iniciar_servidor(ip_memoria,puerto_memoria,logger);
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
    t_registro_tabla_paginas* registro_tabla_paginas;

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


