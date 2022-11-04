#include "../headers/cpu.h"

int main(int argc, char* argv[]){

    validar_argumentos_main(argc);

	char* CONFIG_FILE = argv[1];

    logger = iniciar_logger(LOG_FILE,LOG_NAME);
    cpu_config = iniciar_config(CONFIG_FILE);
    communication_config = init_connection_config();

	char* IP_MEMORIA = config_get_string_value(communication_config,"IP_MEMORIA");
    int PUERTO_MEMORIA = config_get_int_value(communication_config,"PUERTO_MEMORIA");
	char* IP_KERNEL = config_get_string_value(communication_config,"IP_KERNEL");
	int PUERTO_KERNEL = config_get_int_value(communication_config,"PUERTO_KERNEL");

    int socket_memoria = crear_conexion(IP_MEMORIA,PUERTO_MEMORIA);
    enviar_handshake_inicial(socket_memoria,CPU_DISPATCH,logger);

	pthread_create(&thread_escucha_memoria, NULL, conexion_memoria, (void*) (intptr_t)socket_memoria);
    pthread_detach(thread_escucha_memoria);

	int socket_kernel_dispatch = crear_conexion(IP_KERNEL,PUERTO_KERNEL);
    enviar_handshake_inicial(socket_kernel_dispatch,CPU_DISPATCH,logger);

	pthread_create(&thread_escucha_dispatch, NULL, conexion_dispatch, (void*) (intptr_t)socket_kernel_dispatch);
    pthread_detach(thread_escucha_dispatch);

	int socket_kernel_interrupt = crear_conexion(IP_KERNEL,PUERTO_KERNEL);
    enviar_handshake_inicial(socket_kernel_interrupt,CPU_INTERRUPT,logger);

	pthread_create(&thread_escucha_interrupt, NULL, conexion_interrupt, (void*) (intptr_t)socket_kernel_interrupt);
    pthread_detach(thread_escucha_interrupt);

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

void* conexion_memoria(void* socket){
    int socket_memoria = (intptr_t) socket;
    while(socket_memoria != -1){
        op_code codigo_operacion = recibir_operacion(socket_memoria);
	}
	return EXIT_SUCCESS;
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
			enviar_PCB(socket_dispatch,pcb,estado_proceso);
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
		}else{
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
			proceso_respuesta = operacion_MOV_IN(&(instruccion->parametros[0]),instruccion->parametros[1]);
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

op_code operacion_MOV_IN(registro_cpu* registro,uint32_t direccion_logica){
	log_info(logger,string_from_format(CYN"PID: <%d> - Ejecutando <MOV_IN> - <%s> - <%d>"WHT,pcb->pid,traducir_registro_cpu(*registro),direccion_logica));
	(*registro) = direccion_logica;
	return CONTINUA_PROCESO;
}

op_code operacion_MOV_OUT(uint32_t direccion_logica,registro_cpu registro){
	log_info(logger,string_from_format(CYN"PID: <%d> - Ejecutando <MOV_OUT> - <%d> - <%s>"WHT,pcb->pid,direccion_logica,traducir_registro_cpu(registro)));
	direccion_logica = registro;
	return CONTINUA_PROCESO;
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

