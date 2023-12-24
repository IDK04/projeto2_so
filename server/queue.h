#ifndef QUEUE
#define QUEUE

typedef struct Client{
  char req_pipe_path[40], resp_pipe_path[40];
  struct Client* next;
} Client;


typedef struct Queue{
  struct Client* head;  // Head of the queue
  struct Client* tail;  // Tail of the queue
} Queue;

Queue* create_q();

void push(Client *client, Queue *q);

Client* pop(Queue *q);

int empty_q(Queue *q);

void free_q(Queue *q);

void print_q(Queue *q);

#endif  // QUEUE