#include "../headers/shared.h"

t_config* init_connection_config() {
    return iniciar_config(CONNECTION_FILE);
}

int crear_conexion(char* ip, int puerto){
    int socket_cliente = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_cliente == -1) {
        perror("Hubo un error al crear el socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_info;
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(ip);
    server_info.sin_port = htons(puerto);
    memset(&(server_info.sin_zero), '\0', 8); //se rellena con ceros para que tenga el mismo tamaño que socketaddr

    verificar_connect(socket_cliente, &server_info);

    return socket_cliente;
}

void verificar_connect(int socket_cliente, struct sockaddr_in *server_info) {
    if (connect(socket_cliente, (void*) server_info, sizeof((*server_info))) == -1) {
        perror("Hubo un problema conectando al servidor");
        close(socket_cliente);
        exit(EXIT_FAILURE);
    }
}

int iniciar_servidor(char* ip, char* puerto, t_log* logger){
    int socket_servidor;
    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &server_info);

    // Creamos el socket de escucha del kernel
    socket_servidor = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    // Asociamos el socket a un puerto
    verificar_bind(socket_servidor, server_info);
    // Escuchamos las conexiones entrantes
    verificar_listen(socket_servidor);

    freeaddrinfo(server_info);
    log_trace(logger, "Listo para escuchar a mi cliente!");

    return socket_servidor;
}

void verificar_bind(int socket_kernel,  struct addrinfo *kernelinfo){
    uint32_t yes= 1;
    setsockopt(socket_kernel, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    if( bind(socket_kernel, kernelinfo->ai_addr, kernelinfo->ai_addrlen) == -1) {
        perror("Hubo un error en el bind: ");
        close(socket_kernel);
        exit(EXIT_FAILURE);
    }
}

void verificar_listen(int socket){
    if (listen(socket, SOMAXCONN) == -1) {
        perror("Hubo un error en el listen: ");
        close(socket);
        exit(EXIT_FAILURE);
    }
}

int esperar_cliente(int socket_server, t_log* logger){
    // Aceptamos un nuevo cliente
    int socket_cliente = accept(socket_server, NULL, NULL);

    if (socket_cliente == -1) {
        perror("Hubo un error en aceptar una conexión del cliente: ");
        close(socket_server);
        exit(-1);
    }

    log_info(logger, "Se conecto un cliente!");
    return socket_cliente;
}

int enviar_handshake_inicial(int socket, uint32_t id_modulo, t_log* logger){
    uint32_t id_modulo_conectado;
    send(socket,&id_modulo,sizeof(uint32_t),0);
    recv(socket,&id_modulo_conectado,sizeof(uint32_t),MSG_WAITALL);
    if(modulo_valido(id_modulo_conectado)){
        log_info(logger,string_from_format("Conexion establecida: [%s] < ---- > [%s] ",identificadores_modulo(id_modulo),identificadores_modulo(id_modulo_conectado)));
    }else{
        log_error(logger,"Error al establecer conexion entre los modulos");
        exit(EXIT_FAILURE);
    }

    return id_modulo_conectado;
}

int recibir_handshake_inicial(int socket, uint32_t id_modulo, t_log* logger){
    uint32_t id_modulo_conectado;
    
    recv(socket,&id_modulo_conectado,sizeof(uint32_t),MSG_WAITALL);
    if(modulo_valido(id_modulo_conectado)){
        send(socket,&id_modulo,sizeof(uint32_t),0);
        log_info(logger,string_from_format("Conexion establecida: [%s] < ---- > [%s] ",identificadores_modulo(id_modulo),identificadores_modulo(id_modulo_conectado)));
    }else{
        id_modulo = -1;
        send(socket,&id_modulo,sizeof(uint32_t),0);
        log_error(logger,"Error al establecer conexion entre los modulos");
        exit(EXIT_FAILURE);
    }

    return id_modulo_conectado;
}

int modulo_valido(uint32_t modulo){
    return (CONSOLA <= modulo && modulo <= MEMORIA);
}

char* identificadores_modulo(uint32_t id_modulo){
    switch(id_modulo){
        case CONSOLA:       return "CONSOLA";
        case KERNEL:        return "KERNEL";
        case CPU_DISPATCH:  return "CPU DISPATCH";
        case CPU_INTERRUPT: return "CPU INTERRUPT";
        case MEMORIA:       return "MEMORIA";
        default:            return "ERROR MODULO INCORRECTO";
    }
}

t_paquete* crear_paquete(){
    t_paquete* paquete = malloc(sizeof(t_paquete));
    crear_buffer(paquete);
    return paquete;
}

void crear_buffer(t_paquete* paquete){
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = 0;
    paquete->buffer->stream = NULL;
}

void agregar_entero(t_paquete * paquete, uint32_t entero){
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(entero));
    memcpy(paquete->buffer->stream + paquete->buffer->size, &entero, sizeof(entero));
    paquete->buffer->size += sizeof(entero);
}

void agregar_ptr_entero(t_paquete * paquete, uint32_t *entero){
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(entero));
    memcpy(paquete->buffer->stream + paquete->buffer->size, &entero, sizeof(entero));
    paquete->buffer->size += sizeof(entero);
}


void agregar_instruccion(t_paquete* paquete, void* instruccion){
    size_t tamanioOperandos = sizeof(operando)*2;
    int tamanio = sizeof(instr_code)+tamanioOperandos;
    paquete->buffer->stream =
            realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

    memcpy(paquete->buffer->stream + paquete->buffer->size, instruccion, sizeof(instr_code));
    memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(instr_code), instruccion + sizeof(instr_code), tamanioOperandos);
    paquete->buffer->size += tamanio;
}

t_list *deserializar_lista_instrucciones(void *stream, size_t tamanio_lista_instrucciones) {
    int desplazamiento = 0;
    size_t tamanio_instruccion = sizeof(instr_code) + sizeof(operando) * 2;
    t_list *valores = list_create();
    while(desplazamiento < tamanio_lista_instrucciones) {
        void* valor = malloc(tamanio_instruccion);
        memcpy(valor, stream+desplazamiento, tamanio_instruccion);
        desplazamiento += tamanio_instruccion;
        list_add(valores, valor);
    }
    return valores;
}

t_list *deserializar_lista_segmentos(void *stream, size_t tamanio_lista_segmentos) {
    int desplazamiento = 0;
    size_t tamanio_segmento = sizeof(uint32_t);
    t_list *valores = list_create();

    while(desplazamiento < tamanio_lista_segmentos) {
        void* valor = malloc(tamanio_segmento);
        memcpy(valor, stream+desplazamiento, tamanio_segmento);
        desplazamiento += tamanio_segmento;
        list_add(valores, valor);
    }
    return valores;
}


void* serializar_paquete(t_paquete* paquete, size_t bytes){
    void * magic = malloc(bytes);
    int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(op_code));
    desplazamiento+= sizeof(op_code);
    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(size_t));
    desplazamiento+= sizeof(size_t);
    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
    desplazamiento+= paquete->buffer->size;

    return magic;
}

int enviar_paquete(t_paquete* paquete, int socket_destino){
    int tamanio_codigo_operacion = sizeof(op_code);
    int tamanio_stream = paquete->buffer->size;
    size_t tamanio_payload = sizeof(size_t);

    size_t tamanio_paquete = tamanio_codigo_operacion + tamanio_stream + tamanio_payload;
    printf("Paquete [%d] - COP [%d] - Stream [%d] - Payload [%d]\n", tamanio_paquete, tamanio_codigo_operacion, tamanio_stream, tamanio_payload);
    void* a_enviar = serializar_paquete(paquete, tamanio_paquete);

    if(send(socket_destino, a_enviar, tamanio_paquete, 0) == -1){
        perror("Hubo un error enviando el paquete: ");
        free(a_enviar);
        return EXIT_FAILURE;
    }

    free(a_enviar);
    return EXIT_SUCCESS;
}

void eliminar_paquete(t_paquete* paquete){
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

op_code recibir_operacion(int socket_cliente){
    op_code cod_op;

    if(recv(socket_cliente, &cod_op, sizeof(op_code), MSG_WAITALL) > 0) {
        return cod_op;
    } else {
        close(socket_cliente);
        return -1;
    }
}

void* recibir_buffer(int socket_cliente){
    void * buffer;
    size_t streamSize;

    //Recibo el tamaño del mensaje
    recv(socket_cliente, &streamSize, sizeof(size_t), MSG_WAITALL);
    //malloqueo el tamaño del mensaje y lo recibo en buffer
    buffer = malloc(streamSize);
    recv(socket_cliente, buffer, streamSize, MSG_WAITALL);

    return buffer;
}

void enviar_mensaje(char* mensaje, int socket){
    t_paquete *paquete = crear_paquete();
    paquete->codigo_operacion = MENSAJE;
    paquete->buffer->size = strlen(mensaje) + 1;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);
    
    int bytes = paquete->buffer->size + sizeof(op_code) + sizeof(size_t);
    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket, a_enviar, bytes, 0);

    free(a_enviar);
    eliminar_paquete(paquete);
}

void recibir_mensaje(int socket_cliente, t_log* logger){
    char* buffer = recibir_buffer(socket_cliente);
    log_info(logger, "Mensaje: %s", buffer);
    free(buffer);
}

void agregar_lista_instrucciones(t_paquete *paquete, t_list *instrucciones){
    uint32_t cantidad_instrucciones = list_size(instrucciones);
    agregar_entero(paquete, cantidad_instrucciones);
    for(uint32_t i=0; i < list_size(instrucciones); i++){
        t_instruccion *instruccion = list_get(instrucciones, i);
        agregar_instruccion(paquete, (void *) instruccion);
    }
}

void agregar_lista_segmentos(t_paquete* paquete, t_list* segmentos){
    uint32_t cantidad_segmentos = list_size(segmentos);
    printf("\ncantidad de segmentos: %d\n", cantidad_segmentos);
    agregar_entero(paquete, cantidad_segmentos);
    for(uint32_t i=0; i < list_size(segmentos); i++){
       uint32_t segmento = (uint32_t) list_get(segmentos, i);
        agregar_entero(paquete,  segmento);
    }
}

void enviar_lista_instrucciones_segmentos(uint32_t socket_destino, t_list* instrucciones, t_list* segmentos){
	t_paquete* paquete = crear_paquete();
	paquete->codigo_operacion = LISTA_INSTRUCCIONES_SEGMENTOS;

    agregar_lista_instrucciones(paquete, instrucciones);
    agregar_lista_segmentos(paquete, segmentos);
    enviar_paquete(paquete, socket_destino);
    eliminar_paquete(paquete);
}

t_list* recibir_lista_instrucciones(int socket){
    //recibo el tamanio del stream que en este caso es todo lo que viene de consola, instrucciones +segmentos, pero no me sirve de nada ese valor
    size_t tamanio_stream;
    recv(socket, &tamanio_stream, sizeof(size_t), 0);
    //lo que ocupa en bytes una instruccion con sus dos operandos
    size_t tam_lista_instrucciones = sizeof(instr_code)+2*sizeof(operando);
    //recibo por socket la cantidad de instrucciones que me van a llegar;
    uint32_t cantidad_instrucciones;
    recv(socket, &cantidad_instrucciones, sizeof(uint32_t), 0);

    void *stream = malloc(tam_lista_instrucciones*cantidad_instrucciones);
    
    recv(socket, stream, (tam_lista_instrucciones*cantidad_instrucciones), 0);
    t_list* instrucciones = deserializar_lista_instrucciones(stream, (tam_lista_instrucciones * cantidad_instrucciones));
    return instrucciones;
}

t_list* recibir_lista_segmentos(int socket){
    uint32_t cantidad_segmentos;
    recv(socket, &cantidad_segmentos, sizeof(uint32_t), 0);

    size_t tamanio_segmento = sizeof(uint32_t);
    void *stream = malloc(cantidad_segmentos*tamanio_segmento);

    recv(socket, stream, (cantidad_segmentos*tamanio_segmento), 0);

    t_list* segmentos = deserializar_lista_segmentos(stream, (cantidad_segmentos * tamanio_segmento));
    return segmentos;
}

