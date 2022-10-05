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
    uint32_t respuesta_memoria = enviar_handshake_inicial(socket_memoria,CPU_DISPATCH,logger);

	pthread_create(&thread_escucha_memoria, NULL, conexion_memoria, (void*) (intptr_t)socket_memoria);
    pthread_detach(thread_escucha_memoria);

	int socket_kernel_dispatch = crear_conexion(IP_KERNEL,PUERTO_KERNEL);
    uint32_t respuesta_dispatch = enviar_handshake_inicial(socket_kernel_dispatch,CPU_DISPATCH,logger);

	pthread_create(&thread_escucha_dispatch, NULL, conexion_dispatch, (void*) (intptr_t)socket_kernel_dispatch);
    pthread_detach(thread_escucha_dispatch);

	int socket_kernel_interrupt = crear_conexion(IP_KERNEL,PUERTO_KERNEL);
    uint32_t respuesta_interrupt = enviar_handshake_inicial(socket_kernel_interrupt,CPU_INTERRUPT,logger);

	pthread_create(&thread_escucha_interrupt, NULL, conexion_interrupt, (void*) (intptr_t)socket_kernel_interrupt);
    pthread_detach(thread_escucha_interrupt);

	pthread_mutex_init(&mutex_flag_interrupcion,NULL);

	retardo_operacion_cpu = config_get_int_value(cpu_config,"RETARDO_INSTRUCCION");
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
}

void* conexion_dispatch(void* socket){
    int socket_dispatch = (intptr_t) socket;
    while(socket_dispatch != -1){
        op_code codigo_operacion = recibir_operacion(socket_dispatch);	
	}
}

void* conexion_interrupt(void* socket){
    int socket_interrupt = (intptr_t) socket;
    while(socket_interrupt != -1){
        op_code codigo_operacion = recibir_operacion(socket_interrupt);
		if(codigo_operacion == INTERRUPCION) hubo_interrupcion = INTERRUPCION;	
	}
}

//--------Ciclo de instruccion---------
void comenzar_ciclo_instruccion(){

	op_code proceso_respuesta = CONTINUA_PROCESO;
	operando operador = 0;

	log_info(logger,"Iniciando ciclo de instruccion...");

	while(proceso_respuesta == CONTINUA_PROCESO){
		t_instruccion* instruccion = fase_fetch();
		int requiero_operador = fase_decode(instruccion);

		if(requiero_operador) {
			operador = fase_fetch_operand(instruccion->parametros[1]);
		}
		proceso_respuesta = fase_execute(instruccion, operador);
		pthread_mutex_lock(&mutex_flag_interrupcion);
		proceso_respuesta = chequear_interrupcion(proceso_respuesta);
		pthread_mutex_unlock(&mutex_flag_interrupcion);
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
			proceso_respuesta = operacion_SET();
			break;
		case ADD:
			proceso_respuesta = operacion_ADD();
			break;
		case MOV_IN:
			proceso_respuesta = operacion_MOV_IN();
			break;
		case MOV_OUT:
			proceso_respuesta = operacion_MOV_OUT();
			break;
		case IO:
			proceso_respuesta = operacion_IO();
			break;
		case EXIT:
			proceso_respuesta = operacion_EXIT();
			break;
	}
	return proceso_respuesta;
}

op_code operacion_SET(registro_cpu registro,uint32_t valor){
	log_info(logger,string_from_format("PID: <%d> - Ejecutando <SET> - <%d> - <%d>",pcb->pid,registro,valor));
	return CONTINUA_PROCESO;
}
