#include "../headers/planification.h"

void init_planification(){
    pthread_t dispatcher;
    pthread_t long_planner;

    pthread_create(&long_planner, NULL, long_planner, NULL);
    pthread_create(&dispatcher, NULL, dispatcher, NULL);
}

void init_all_mutex(){
    pthread_mutex_init(&mutex_new,NULL);
    pthread_mutex_init(&mutex_ready,NULL);
    pthread_mutex_init(&mutex_running,NULL);
    pthread_mutex_init(&mutex_blocked_screen,NULL);
    pthread_mutex_init(&mutex_blocked_keyboard,NULL);
    pthread_mutex_init(&mutex_blocked_page_fault,NULL);
    pthread_mutex_init(&mutex_blocked_io,NULL);
    pthread_mutex_init(&mutex_exit,NULL);
}

void push_pcb_to_queue(t_pcb* pcb,pthread_mutex_t mutex, t_queue* cola){
    pthread_mutex_lock(&mutex);
    queue_push(cola,pcb);
    pthread_mutex_lock(&mutex);
}

t_pcb* pop_pcb_from_queue(pthread_mutex_t mutex, t_queue* cola){
    pthread_mutex_lock(&mutex);
    t_pcb* pcb = queue_pop(cola);
    pthread_mutex_lock(&mutex);
    return pcb;
}

void long_planner(){
    pthread_t process_terminator_thread;
    pthread_create(&process_terminator_thread, NULL, process_terminator, NULL);
    pcb_to_dispatcher();
    pthread_join(&process_terminator,NULL);
}

void pcb_to_dispatcher(){
    while(1){
        sem_wait(&new_to_ready);
        sem_wait(&grado_multiprogramacion);
        t_pcb* pcb = pop_pcb_from_queue(mutex_new,new_queue);
        push_pcb_to_queue(pcb,mutex_ready,ready_queue);
        sem_post(&ready_to_running);
    }
}

void process_terminator(){
    while(1){
        sem_wait(&finish_process);
        t_pcb* pcb = pop_pcb_from_queue(mutex_exit,exit_queue);
        free(pcb);
        //Avisar a memoria
        //Avisar a consola
    }
}

void dispatcher(){
    pthread_t ready_thread;
    pthread_t blocked_page_fault_thread;
    pthread_t blocked_io_thread;
    pthread_t blocked_keyboard_thread;
    pthread_t blocked_screen_thread;
    pthread_t running_thread;

    pthread_create(&ready_thread, NULL, ready_handler, NULL);
    pthread_create(&running_thread, NULL, ready_handler, NULL);

    pthread_join(&ready_thread,NULL);

}

void ready_handler(){
    while(1){
        sem_wait(&ready_to_running);
        t_pcb* pcb = pop_pcb_from_queue(mutex_ready,ready_queue);
        push_pcb_to_queue(pcb,mutex_running,running_queue);
        sem_post(&execute_process);
    }
}

void running_handler(){
    while(1){
        sem_wait(&execute_process);
        //Enviar pcb a cpu
    }
}