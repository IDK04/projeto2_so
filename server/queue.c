#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

Queue* create_q(){
    Queue* q = (Queue*) malloc(sizeof(struct Queue));
    if(!q) return NULL;
    q->head = NULL;
    q->tail = NULL;
    return q;
}

void push(Client *client, Queue *q){
    if(q == NULL) return;

    client->next = NULL;

    if(q->head == NULL){
        q->head = client;
        q->tail = client;
        return;
    }
    
    q->tail->next = client;
    q->tail = client;
}

Client *pop(Queue *q){
    if(q == NULL) return NULL;
    if(q->head == NULL) return NULL;

    Client *c = q->head;

    q->head = q->head->next;
    if(q->head == NULL)
        q->tail = NULL;

    return c;
}

int empty_q(Queue *q){
    if(q == NULL) return 0;
    return q->head==NULL;
}

void free_q(Queue *q){
    if(q == NULL) return;

    Client *aux;

    while(q->head != NULL){
        aux = pop(q);
        free(aux);
    }

    free(q);
}

void print_q(Queue *q){
    if(q == NULL) return;

    Client *aux = q->head;

    while (aux != NULL){
        printf("Client req:%s;res:%s\n", aux->req_pipe_path, aux->resp_pipe_path);
        aux = aux->next;
    }

}