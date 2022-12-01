
#include "../headers/cpu.h"

void imprimir_entradas_tlb();

int main(int argc, char* argv[]){

    validar_argumentos_main(argc);

	char* CONFIG_FILE = argv[1];

    logger = iniciar_logger(LOG_FILE,LOG_NAME);
    cpu_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();
    tlb = list_create();
	lista_dispositivos = config_get_array_value(cpu_config,"DISPOSITIVOS_IO");

	char* IP_MEMORIA = config_get_string_value(communication_config,"IP_MEMORIA");
    int PUERTO_MEMORIA = config_get_int_value(communication_config,"PUERTO_MEMORIA");
	char* IP_KERNEL = config_get_string_value(communication_config,"IP_KERNEL");
	int PUERTO_KERNEL = config_get_int_value(communication_config,"PUERTO_KERNEL");
    algoritmo_reemplazo_tlb = config_get_string_value(cpu_config, "REEMPLAZO_TLB");
    entradas_max_tlb = config_get_int_value(cpu_config, "ENTRADAS_TLB");

    socket_kernel_dispatch = crear_conexion(IP_KERNEL,PUERTO_KERNEL);
    enviar_handshake_inicial(socket_kernel_dispatch,CPU_DISPATCH,logger);
    pthread_create(&thread_escucha_dispatch, NULL, conexion_dispatch, (void*) (intptr_t) socket_kernel_dispatch);
    pthread_detach(thread_escucha_dispatch);

	socket_kernel_interrupt = crear_conexion(IP_KERNEL,PUERTO_KERNEL);
    enviar_handshake_inicial(socket_kernel_interrupt,CPU_INTERRUPT,logger);
	pthread_create(&thread_escucha_interrupt, NULL, conexion_interrupt, (void*) (intptr_t) socket_kernel_interrupt);
    pthread_detach(thread_escucha_interrupt);

    socket_memoria = crear_conexion(IP_MEMORIA,PUERTO_MEMORIA);
    handshake_memoria(socket_memoria);

	pthread_mutex_init(&mutex_flag_interrupcion,NULL);

	retardo_operacion_cpu = config_get_int_value(cpu_config,"RETARDO_INSTRUCCION");
	log_info(logger,string_from_format("Retardo por operacion de CPU: %ds",retardo_operacion_cpu/1000));
	printf("\n");
	retardo_operacion_cpu = retardo_operacion_cpu*1000;

	sem_init(&desalojar_pcb,0,0);
	sem_init(&continuar_ciclo_instruccion,0,0);

	sem_wait(&continuar_ciclo_instruccion);
	comenzar_ciclo_instruccion();

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

void* conexion_dispatch(void* socket){
    int socket_dispatch = (intptr_t) socket;
    while(socket_dispatch != -1){
        op_code codigo_operacion = recibir_operacion(socket_dispatch);
		if(codigo_operacion == PCB){
			pcb = recibir_PCB(socket_dispatch);
			log_info(logger,string_from_format(BLU"Conexion Dispatch: PCB Recibido: PID <%d>"WHT,pcb->pid));
			sem_post(&continuar_ciclo_instruccion);
			sem_wait(&desalojar_pcb);
			enviar_PCB(socket_dispatch, pcb,estado_proceso);
			log_info(logger,string_from_format(BLU"Conexion Dispatch: PCB Enviado: PID <%d>"WHT,pcb->pid));
			pcb = NULL;
		}else{
			log_warning(logger,"Conexion Dispatch -> Recibio una operacion incorrecta");
			printf(YEL"\t > Codigo Operacion: %d\n",codigo_operacion);
			
			exit(EXIT_FAILURE);
		}
	}    
	return EXIT_SUCCESS;
}

void* conexion_interrupt(void* socket){
    int socket_interrupt = (intptr_t) socket;
    while(socket_interrupt != -1){
        op_code codigo_operacion = recibir_operacion(socket_interrupt);

		if(codigo_operacion == INTERRUPCION) {
			log_info(logger,BLU"Conexion Interrupt: Interrupcion recibida"WHT);
			pthread_mutex_lock(&mutex_flag_interrupcion);
			hubo_interrupcion = INTERRUPCION;
			pthread_mutex_unlock(&mutex_flag_interrupcion);
		}else{
			log_warning(logger,"Conexion Interrupt -> Recibio una operacion incorrecta");
			printf(YEL"\t > Codigo Operacion: %d\n"WHT,codigo_operacion);
			exit(EXIT_FAILURE);
		}
	}
	return EXIT_SUCCESS;
}

//--------Ciclo de instruccion---------
void comenzar_ciclo_instruccion(){

	estado_proceso = CONTINUA_PROCESO;
	operando operador = 0;

	log_info(logger,"Iniciando ciclo de instruccion...");

	while(estado_proceso == CONTINUA_PROCESO){
		t_instruccion* instruccion = fase_fetch();
		int requiero_operador = fase_decode(instruccion);

		if(requiero_operador) {
			operador = fase_fetch_operand(instruccion->parametros[1]);
		}
		estado_proceso = fase_execute(instruccion, operador);
		if(estado_proceso == CONTINUA_PROCESO){
			chequear_interrupcion();
		} else {
			desalojo_proceso();
		}
	}

}

t_instruccion* fase_fetch(){
	t_instruccion* instruccion = list_get(pcb->lista_instrucciones, pcb->program_counter);
	pcb->program_counter++;
	return instruccion;
}

int fase_decode(t_instruccion* instruccion){
	instr_code cop = instruccion->codigo_operacion;
	if(cop == SET || cop == ADD) usleep(retardo_operacion_cpu);
	return (cop == MOV_IN || cop == MOV_OUT);
}

operando fase_fetch_operand(operando direccion_operador_a_buscar) {
	return direccion_operador_a_buscar;
}

op_code fase_execute(t_instruccion* instruccion, uint32_t operador){
	op_code proceso_respuesta = CONTINUA_PROCESO;
	switch(instruccion->codigo_operacion){
		case SET:
			proceso_respuesta = operacion_SET(instruccion->parametros[0],instruccion->parametros[1]);
			break;
		case ADD:
			proceso_respuesta = operacion_ADD(instruccion->parametros[0],instruccion->parametros[1]);
			break;
		case MOV_IN:
			proceso_respuesta = operacion_MOV_IN(instruccion->parametros[0],instruccion->parametros[1]);
			break;
		case MOV_OUT:
			proceso_respuesta = operacion_MOV_OUT(instruccion->parametros[0],instruccion->parametros[1]);
			break;
		case IO:
			proceso_respuesta = operacion_IO(instruccion->parametros[0],instruccion->parametros[1]);
			break;
		case EXIT:
			proceso_respuesta = operacion_EXIT();
			break;
	}
	return proceso_respuesta;
}

op_code operacion_SET(registro_cpu registro,uint32_t valor){
	log_info(logger,string_from_format(CYN"PID: <%d> - Ejecutando <SET> - <%s> - <%d>"WHT,pcb->pid,traducir_registro_cpu(registro),valor));
	registro_cpu* registro_pcb = obtener_registro(pcb,registro); 
	(*registro_pcb) = valor;
	return CONTINUA_PROCESO;
}

op_code operacion_ADD(registro_cpu registro1,registro_cpu registro2){
	log_info(logger,string_from_format(CYN"PID: <%d> - Ejecutando <ADD> - <%s> - <%s>"WHT,pcb->pid,traducir_registro_cpu(registro1),traducir_registro_cpu(registro2)));
	registro_cpu* registro_pcb1 = obtener_registro(pcb,registro1); 
	registro_cpu* registro_pcb2 = obtener_registro(pcb,registro2); 
	(*registro_pcb1) = (*registro_pcb1 + *registro_pcb2);
	return CONTINUA_PROCESO;
}

/** MOV_IN (Registro, Dirección Lógica): Lee el valor de memoria del segmento de Datos correspondiente
 * a la Dirección Lógica y lo almacena en el Registro.
 */

op_code operacion_MOV_IN(registro_cpu registro, uint32_t direccion_logica){
	log_info(logger,string_from_format(CYN"PID: <%d> - Ejecutando <MOV_IN> - <%s> - <%d>"WHT, pcb->pid,traducir_registro_cpu(registro),direccion_logica));
    punteros_cpu* punteros_cpu = obtener_direccion_fisica(direccion_logica);

    if(punteros_cpu != NULL) {
        uint32_t valor = leer_en_memoria(punteros_cpu);
        registro_cpu* registro_pcb = obtener_registro(pcb, registro);
        (*registro_pcb) = valor;
        log_info(logger, "%s",
                 string_from_format("PID: <%d> - Acción: <LEER> - Segmento: <%d> - Pagina: <%d> - Dirección Fisica: <marco:%d-despl:%d>”",
                                    pcb->pid, punteros_cpu->numero_segmento, punteros_cpu->numero_pagina, punteros_cpu->direccion_fisica->marco, punteros_cpu->direccion_fisica->desplazamiento));

        return CONTINUA_PROCESO;
    }

	return estado_proceso;
}

op_code operacion_MOV_OUT(uint32_t direccion_logica, registro_cpu registro){
	log_info(logger,string_from_format(CYN"PID: <%d> - Ejecutando <MOV_OUT> - <%d> - <%s>"WHT,pcb->pid,direccion_logica,traducir_registro_cpu(registro)));
    punteros_cpu * punteros_cpu = obtener_direccion_fisica(direccion_logica);

    if(punteros_cpu != NULL) {
        registro_cpu* registro_pcb = obtener_registro(pcb,registro);
        uint32_t valor = (*registro_pcb);
        escribir_en_memoria(punteros_cpu, valor);
        log_info(logger, "%s",
                 string_from_format(BLU"PID: <%d> - Acción: <ESCRIBIR> - Segmento: <%d> - Pagina: <%d> - Dirección Fisica: <marco:%d-despl:%d>"RESET,
                                    pcb->pid, punteros_cpu->numero_segmento, punteros_cpu->numero_pagina, punteros_cpu->direccion_fisica->marco, punteros_cpu->direccion_fisica->desplazamiento));
        return CONTINUA_PROCESO;
    }

    return estado_proceso;
}

op_code operacion_IO(dispositivo dispositivo,uint32_t unidades_trabajo){
	switch (dispositivo){
		case PANTALLA:
			log_info(logger,string_from_format(CYN"PID: <%d> - Ejecutando <IO> - <%s> - <%s>"WHT,pcb->pid,traducir_dispositivo(dispositivo),traducir_registro_cpu(unidades_trabajo)));
			return BLOQUEAR_PROCESO_PANTALLA;
		case TECLADO:
			log_info(logger,string_from_format(CYN"PID: <%d> - Ejecutando <IO> - <%s> - <%s>"WHT,pcb->pid,traducir_dispositivo(dispositivo),traducir_registro_cpu(unidades_trabajo)));
			return BLOQUEAR_PROCESO_TECLADO;
		default:
			log_info(logger,string_from_format(CYN"PID: <%d> - Ejecutando <IO> - <%s> - <%d>"WHT,pcb->pid,traducir_dispositivo(dispositivo),unidades_trabajo));
			return BLOQUEAR_PROCESO_IO;
	}
}

op_code operacion_EXIT(){
	log_info(logger,string_from_format(CYN"PID: <%d> - Ejecutando <EXIT>"WHT,pcb->pid));
	return FINALIZAR_PROCESO;
}

void chequear_interrupcion(){
	printf("\n");
	log_info(logger,"Check de interrupciones...");

	if(hubo_interrupcion == INTERRUPCION){
		estado_proceso = INTERRUPCION;
		desalojo_proceso();
	}

}

void desalojo_proceso(){
	sem_post(&desalojar_pcb);
	sem_wait(&continuar_ciclo_instruccion);
	estado_proceso = CONTINUA_PROCESO;
	hubo_interrupcion = CONTINUA_PROCESO;
}

punteros_cpu * obtener_direccion_fisica(uint32_t direccion_logica) {

    uint32_t tamanio_maximo_segmento = entradas_por_tabla * tamanio_pagina;
    uint32_t numero_segmento = floor(direccion_logica / tamanio_maximo_segmento);
    uint32_t desplazamiento_segmento = direccion_logica % tamanio_maximo_segmento;
    uint32_t numero_pagina = floor(desplazamiento_segmento / tamanio_pagina);
    uint32_t desplazamiento_pagina = desplazamiento_segmento % tamanio_pagina;

    int marco;
    t_segmento* segmento = list_get(pcb->tabla_segmentos, numero_segmento);

    if (desplazamiento_segmento > segmento->tamanio_segmento) {
        log_info(logger, string_from_format(RED"PID:<%d> - SEGMENTATION FAULT - SEGMENTO: <%d> - PAGINA: <%d>"RESET, pcb->pid, numero_segmento, numero_pagina));
        estado_proceso = SEGMENTATION_FAULT;
        return NULL;
    }

    marco = tlb_obtener_marco(pcb->pid, numero_segmento, numero_pagina);
    if (marco == -1 ) {
        log_info(logger, "%s", string_from_format(RED"PID:<%d> - TLB MISS - SEGMENTO: <%d> - PAGINA: <%d>"RESET, pcb->pid, numero_segmento, numero_pagina));
        uint32_t indice_tabla_paginas = ((t_segmento*) (list_get(pcb->tabla_segmentos, numero_segmento)))->indice_tabla_paginas;
        marco = obtener_marco_memoria(indice_tabla_paginas, numero_pagina);
        if (marco == -1) {
            log_info(logger, "%s", string_from_format(RED"Page Fault PID: <%d> - Segmento: <%d> - Pagina: <%d>"RESET, pcb->pid, numero_segmento, numero_pagina));
            return NULL;
        }
        tlb_actualizar(pcb->pid, numero_segmento, numero_pagina, marco);
    } else {
        log_info(logger, "%s", string_from_format(GRN"PID:<%d> - TLB HIT - SEGMENTO: <%d> - PAGINA: <%d>"RESET, pcb->pid, numero_segmento, numero_pagina));
     }

    punteros_cpu *punteros_cpu = malloc(sizeof (punteros_cpu));
    punteros_cpu->pid = pcb->pid;
    punteros_cpu->numero_segmento = numero_segmento;
    punteros_cpu->indice_tabla_paginas = segmento->indice_tabla_paginas;
    punteros_cpu->numero_pagina = numero_pagina;
    dir_fisica * direccion_fisica = malloc(sizeof(dir_fisica));
    direccion_fisica->marco = marco;
    direccion_fisica->desplazamiento = desplazamiento_pagina;
    punteros_cpu->direccion_fisica = direccion_fisica;

    return punteros_cpu;
}

void handshake_memoria(int conexion_memoria){
    op_code codigo_operacion = recibir_operacion(conexion_memoria);
    size_t tamanio_stream;
    if (codigo_operacion == HANDSHAKE_CPU_MEMORIA) {
        recv(conexion_memoria, &tamanio_stream, sizeof(size_t), 0); // no me importa en este caso
        recv(conexion_memoria, &tamanio_pagina, sizeof(uint32_t), 0);
        recv(conexion_memoria, &entradas_por_tabla, sizeof(uint32_t), 0);
    }
}

////--------------------------------------------------------TLB------------------------------------------------------------------

int tlb_obtener_marco(uint32_t pid, uint32_t numero_segmento, uint32_t numero_pagina) {
    tlb_entrada * entrada_tlb = malloc(sizeof(tlb_entrada));
    uint32_t instante_referencia = 0; 

    if (list_size(tlb) > 0) {
        for (int i=0; i < list_size(tlb); i++) {
            entrada_tlb = (tlb_entrada *) list_get(tlb,i);
            if (entrada_tlb->pid &&  entrada_tlb->segmento == numero_segmento && entrada_tlb->pagina == numero_pagina) {
                entrada_tlb->instante_referencia = instante_referencia + 1;
                return entrada_tlb->marco;
            }
            instante_referencia = (instante_referencia > entrada_tlb->instante_referencia) ? instante_referencia : entrada_tlb->instante_referencia;
        }
    }
    return -1;
}

void reemplazar_entrada_tlb(tlb_entrada* entrada) {
    if (strcmp(algoritmo_reemplazo_tlb, "FIFO") ==0){
        list_remove(tlb, 0);
        list_add(tlb, entrada);
    }
    else {
        /*
        list_sort(tlb, comparator);
        list_remove(tlb, 0);
        list_add(tlb, entrada);
        */
        uint32_t instante_referencia_nueva_entrada = 0;
        uint32_t indice_victima = obtener_indice_entrada_menor_instante_referencia(&instante_referencia_nueva_entrada);
        //Actualizo con el instante de referencia correspondiente para la entrada nueva
        entrada->instante_referencia = instante_referencia_nueva_entrada + 1;
        list_remove(tlb,indice_victima);
        list_add_in_index(tlb,indice_victima,entrada);
    }
}

uint32_t obtener_indice_entrada_menor_instante_referencia(uint32_t* instante_referencia_nueva_entrada){

    uint32_t indice = 0;
    tlb_entrada* entrada_i = list_get(tlb, 0);

    uint32_t instante_referencia_minimo = entrada_i->instante_referencia;

    for(uint32_t i=0; i < list_size(tlb); i++){
        entrada_i = list_get(tlb, i);
        
        //Obtengo el instante de referencia mas chico para reemplazar esa entrada
        if(instante_referencia_minimo > entrada_i->instante_referencia){
            instante_referencia_minimo = entrada_i->instante_referencia;
            indice = i;
        }

        //Obtengo el instante de referencia mas alto para la nueva entrada
        if(*instante_referencia_nueva_entrada < entrada_i->instante_referencia){
            *instante_referencia_nueva_entrada = entrada_i->instante_referencia;
        }
    }
    return indice;
}

void tlb_actualizar(uint32_t pid, uint32_t numero_segmento, uint32_t numero_pagina, uint32_t marco){
    log_info(logger, "\nNUMERO_SEGMENTO: %d", numero_segmento);
    tlb_entrada* tlb_entrada = malloc(sizeof(uint32_t)*5);
    tlb_entrada->pid = pid;
    tlb_entrada->segmento = numero_segmento;
    tlb_entrada->pagina = numero_pagina;
    tlb_entrada->marco = marco;
    tlb_entrada->instante_referencia=1;
    if (list_size(tlb) >= entradas_max_tlb){
        reemplazar_entrada_tlb(tlb_entrada);
    } else {
        list_add(tlb, tlb_entrada);
    }
    imprimir_entradas_tlb();
}

void imprimir_entradas_tlb() {
    for (int i=0; i < tlb->elements_count; i++) {
        tlb_entrada* entrada = list_get(tlb, i);
       log_info(logger,string_from_format(YEL"<ENTRADA_TLB:%d>|PID:<%d>|SEGMENTO:<%d>|PAGINA:<%d>|MARCO:<%d>"RESET,
                                          i,
                                          entrada->pid,
                                          entrada->segmento,
                                          entrada->pagina,
                                          entrada->marco));
    }
}

static bool comparator (void* entrada1, void* entrada2) {
    return (((tlb_entrada *) entrada1)->instante_referencia) < (((tlb_entrada *) entrada2)->instante_referencia);
}

void limpiar_tlb(){
    list_clean(tlb);
}

/*
 * Si el marco que me viene de memoria ya es una entrada en la tlb con otra pagina, le actualizo la página
 * */
void actualizar_entrada_marco_existente(uint32_t numero_pagina, uint32_t marco){
    tlb_entrada * entrada;
    for(int i=0; i< list_size(tlb);i++) {
        entrada = list_get(tlb, i);
        if (entrada->marco == marco && numero_pagina!= entrada->pagina) {
            entrada->pagina = numero_pagina;
            entrada->instante_referencia=1;
        }
    }
}

uint32_t leer_en_memoria(punteros_cpu * punteros_cpu) {
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = LEER_MEMORIA;
    agregar_entero(paquete, punteros_cpu->pid);
    agregar_entero(paquete, punteros_cpu->indice_tabla_paginas);
    agregar_entero(paquete, punteros_cpu->numero_pagina);
    agregar_entero(paquete, punteros_cpu->direccion_fisica->marco);
    agregar_entero(paquete, punteros_cpu->direccion_fisica->desplazamiento);

    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    uint32_t valor_leido;
    int obtuve_valor = 0;
    while (socket_memoria != -1 && obtuve_valor == 0) {
        op_code cod_op = recibir_operacion(socket_memoria);
        if(cod_op == LEER_MEMORIA){
            void* buffer = recibir_buffer(socket_memoria);
            memcpy(&valor_leido, buffer, sizeof(uint32_t));
            obtuve_valor = 1;
        }
    }
    return valor_leido;

}

int obtener_marco_memoria(uint32_t indice_tabla_paginas, uint32_t numero_pagina) {

    t_paquete * paquete = crear_paquete();
    paquete->codigo_operacion = OBTENER_MARCO;
    agregar_entero(paquete, pcb->pid);
    agregar_entero(paquete, indice_tabla_paginas);
    agregar_entero(paquete, numero_pagina);
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    int marco = -1;

    if(socket_memoria != -1){
        op_code cod_op = recibir_operacion(socket_memoria);
        switch(cod_op) {
            case OBTENER_MARCO:
                ;
                marco = recibir_valor(socket_memoria);
                break;
            case PAGE_FAULT:
                pcb->program_counter--;
                estado_proceso=PAGE_FAULT;
                break;
            default:
                log_warning(logger,"CPU -> Memoria envio algo mal");
                break;
    }
    return marco;
    }
}

void escribir_en_memoria(punteros_cpu * punteros_cpu, uint32_t valor) {
    t_paquete* paquete = crear_paquete();
    paquete->codigo_operacion = ESCRIBIR_MEMORIA;
    agregar_entero(paquete, pcb->pid);
    agregar_entero(paquete, punteros_cpu->numero_pagina); // lo necesito para actualizar el proceso en swap
    agregar_entero(paquete, punteros_cpu->direccion_fisica->marco);
    agregar_entero(paquete, punteros_cpu->direccion_fisica->desplazamiento);
    agregar_entero(paquete, punteros_cpu->indice_tabla_paginas);
    agregar_entero(paquete, valor);

    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    int recibi_mensaje = 0;
    while (socket_memoria != -1 && recibi_mensaje == 0) {
        op_code cod_op = recibir_operacion(socket_memoria);
        if(cod_op == MENSAJE){
            recibir_mensaje(socket_memoria, logger);
            recibi_mensaje = 1;
        }
    }
}


