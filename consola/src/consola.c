int main(int argc, char* argv[]) {

    if(argc < 4){
        printf(RED"");
        printf("Cantidad de parametros incorrectos.\n");
        printf("1- Ruta al archivo con instrucciones a ejecutar.\n2- Tamaño del proceso\n3- Ruta del archivo de configuracion\n");
        printf(RESET"");
        return argc;
    }

    CONFIG_FILE = argv[3];

    logger = iniciarLogger(LOG_FILE, LOG_NAME);
    config = iniciarConfig(CONFIG_FILE);
    char* ip = config_get_string_value(config,"IP_KERNEL");
    int puerto = config_get_int_value(config,"PUERTO_KERNEL");

    conexionKernel = crearConexion(ip, puerto, "Consola");

    char* rutaArchivo = argv[1];
    int tamanioProceso = atoi(argv[2]);

    t_list* listaInstrucciones = parsearInstrucciones(logger, rutaArchivo);
    enviarListaInstrucciones(conexionKernel, tamanioProceso, listaInstrucciones);

    while(conexionKernel!=-1){

        op_code cod_op = recibirOperacion(conexionKernel);
        switch (cod_op) {
            case MENSAJE:
                recibirMensaje(conexionKernel, logger);
                break;
            case TERMINAR_PROCESO:
                printf(GRN"");
                log_info(logger,"TERMINANDO PROGRAMA");
                printf(RESET"");
                terminarPrograma(conexionKernel, logger, config);
                return EXIT_SUCCESS;
                break;
            default:
                log_trace(logger, "Operación desconocida en consola");
                break;
        }
    }
    return EXIT_SUCCESS;
}

t_list* parsearInstrucciones(t_log* logger, char* rutaArchivo) {

    t_list* listaInstrucciones = list_create();

    char** lineasPseudocodigo = leer_archivo_pseudocodigo(rutaArchivo, logger);
    if(lineasPseudocodigo == NULL) {
        log_error(logger,"Lineas Pseudocodigo -> NULL");
    }

    for(uint32_t i=0; i< string_array_size(lineasPseudocodigo); i++){
        printf("%s",lineasPseudocodigo[i]);
    }
    printf("\n");

    generarListaInstrucciones(&listaInstrucciones, lineasPseudocodigo);
    string_array_destroy(lineasPseudocodigo);
    return listaInstrucciones;
}


void generarListaInstrucciones(t_list** instrucciones, char** pseudocodigo){
    while(!string_array_is_empty(pseudocodigo)){
        char* instr = string_array_pop(pseudocodigo);
        agregarInstrucciones(instrucciones, instr);
        free(instr);
    }
}

void agregarInstrucciones(t_list** instrucciones, char* itr){
    uint32_t posicion = 0;
    t_instruccion* instruccion = malloc(sizeof(t_instruccion));
    char* operacion = strtok_r(itr," ",&itr);
    instruccion->codigo_operacion = obtener_cop(operacion);
    switch(instruccion->codigo_operacion){
        case NO_OP:
            instruccion->parametros[0]=0;
            instruccion->parametros[1]=0;
            char* str = strtok_r(NULL,"\n",&itr);
            uint32_t repeticiones = atoi(str);
            for(uint32_t i=0; i<repeticiones; i++) list_add_in_index(*instrucciones,posicion,instruccion);
            break;
        case IO: case READ:
            instruccion->parametros[0]=atoi(strtok_r(NULL,"\n",&itr));
            instruccion->parametros[1]=0;
            list_add_in_index(*instrucciones,posicion,instruccion);
            break;
        case COPY: case WRITE:
            instruccion->parametros[0]=atoi(strtok_r(NULL," ",&itr));
            instruccion->parametros[1]=atoi(strtok_r(NULL,"\n",&itr));
            list_add_in_index(*instrucciones,posicion,instruccion);
            break;
        case EXIT:
            instruccion->parametros[0]=0;
            instruccion->parametros[1]=0;
            list_add_in_index(*instrucciones,posicion,instruccion);
            break;
    }
}

char** leer_archivo_pseudocodigo(char* ruta, t_log* logger){
    uint32_t tamMaximo = 18;
    char** lineas = string_array_new();
    char linea[tamMaximo];
    FILE* archivo = fopen(ruta,"r");
    if(archivo!=NULL){
        log_info(logger,"Archivo abierto exitosamente");
        uint32_t i=0;
        while (fgets(linea,tamMaximo,archivo) != NULL) {
            string_array_push(&lineas,linea);
            lineas[i] = string_duplicate(linea);
            i++;
        }
        fclose(archivo);
        return lineas;
    }else{
        log_error(logger,"ERROR al abrir archivo PSEUDOCODIGO - Posible error en la ruta");
        return NULL;
    }
}


instr_code obtener_cop(char* operacion){
    if(string_contains(operacion,"NO_OP")) 		return NO_OP;
    else if(string_contains(operacion,"I/O")) 	return IO;
    else if(string_contains(operacion,"READ")) 	return READ;
    else if(string_contains(operacion,"COPY")) 	return COPY;
    else if(string_contains(operacion,"WRITE")) return WRITE;
    else return EXIT;
}


void enviarListaInstrucciones(uint32_t conexion, int tamanioProceso, t_list* instrucciones) {
    t_paquete* paquete = crearPaquete();
    paquete->codigo_operacion = LISTA_INSTRUCCIONES;

    agregarListaInstrucciones(paquete, instrucciones);
    agregarTamanioProceso(paquete, tamanioProceso);
    enviarPaquete(paquete, conexion);
    eliminarPaquete(paquete);

}


