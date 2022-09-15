#include "shared.h"

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
    uint32_t respuesta;
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
    return !(string_contains(identificadores_modulo(modulo),"ERROR"));
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