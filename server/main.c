#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "serverparser.h"
#include "queue.h"

Queue *q;
int active_clients = 0;

pthread_cond_t active_clients_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty_q_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;

int process_request(int req_fd, int resp_fd){
  int session_id_client,res;
  unsigned int event_id;
  size_t num_rows, num_cols, num_seats, xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
  ssize_t bytes_written;

  switch (get_next_req(req_fd)){

    case SETUP: //Will never be SETUP
      break;
      
    case QUIT:
      if (parse_session_id(req_fd, &session_id_client)){
        return 0;
      }
      printf("O cliente %d faleceu\n", session_id_client);
      return 0;
    
    case CREATE:
      if(parse_create(req_fd, &event_id, &num_rows, &num_cols, &session_id_client)){
        return 0;
      }
      res = ems_create(event_id,num_rows,num_cols);
      bytes_written = write(resp_fd, &res, sizeof(int));
      if (bytes_written < 0)
        return 0;
      if(res)
        return 0;
      printf("O cliente %d mandou criar e eu criei...\n",session_id_client);
      break;


    case RESERVE:
      if(parse_reserve(req_fd, &event_id, &num_seats,xs,ys,&session_id_client)){
          return 0;
      }
      res = ems_reserve(event_id, num_seats, xs, ys);
      bytes_written = write(resp_fd, &res, sizeof(int));
      if (bytes_written < 0)
        return 0;
      if(res)
        return 0;
      printf("Esse cao: %d mandou reservar %lu seats, fds...\n",session_id_client,num_seats);
      break;

    case SHOW:
      if(parse_show(req_fd,&event_id,&session_id_client)){
          return 0;
      }
      // por fazer (seguir protocolo: res | rows | cols | seats )
      // alterar argumentos e função ems_show, escrevemos para o fdout no cliente como está la comentado
      // atentar na ordem e como vamos fazer
      printf("O laia printou-mos :%d",session_id_client);
      break;

    case LIST_EVENTS:
      break;
      
    case EOC:
      break;
    
    default:
      break;
  }
  return 1;
}

void *clientThread(void *arguments){
  int *parsed_arguments = (int*) arguments;
  int session_id = *parsed_arguments;
  free(parsed_arguments);

  while(1){
    if (pthread_mutex_lock(&cond_mutex) != 0) {return NULL;}

    while (empty_q(q)) {
      pthread_cond_wait(&empty_q_cond, &cond_mutex);
    }

    while(active_clients>=MAX_SESSION_COUNT){
      pthread_cond_wait(&active_clients_cond, &cond_mutex);
    }

    active_clients++;
    Client *client = pop(q);
    if (pthread_mutex_unlock(&cond_mutex) != 0) {return NULL;}

    if (!client)
      return NULL;

    int resp_fd = open(client->resp_pipe_path, O_WRONLY);
    if(resp_fd < 0)
      return NULL;

    if (write(resp_fd, &session_id, sizeof(int)) < 0) {
      return NULL;
    }

    int req_fd = open(client->req_pipe_path, O_RDONLY);
    if(resp_fd < 0){
      return NULL;
    }

    while(process_request(req_fd, resp_fd));

    close(req_fd);
    close(resp_fd);    
    free(client);

    if (pthread_mutex_lock(&cond_mutex) != 0) {return NULL;}
    active_clients--;
    if (pthread_mutex_unlock(&cond_mutex) != 0) {return NULL;}
  }

  return NULL;
}

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }
  
  if(!(q = create_q())){
    fprintf(stderr, "Failed to initialize queue\n");
    return 1;
  }

  unlink(argv[1]);

  if(mkfifo(argv[1],0777)<0){
    fprintf(stderr, "Failed to initialize the server\n");
    return 1;
  }

  int rx = open(argv[1], O_RDONLY);
  if (rx == -1) {
      fprintf(stderr, "Server open failed: %s\n", strerror(errno));
      return 1;
  }

  pthread_t tid[MAX_SESSION_COUNT];

  for(int i = 0; i < MAX_SESSION_COUNT; i++){
    int *id = (int*) malloc(sizeof(int));
    *id = i;
    if(pthread_create(&tid[i], 0, clientThread, id) != 0){
      fprintf(stderr, "Error creating thread\n");
      return 1;
    }
  }

  while (1) {

    char req_pipe_path[MAX_PIPE_PATH_SIZE], resp_pipe_path[MAX_PIPE_PATH_SIZE];
    enum Request req = get_next_req(rx); // will either be SETUP or EOC
    if (req == SETUP){
      if (parse_setup(rx, req_pipe_path, resp_pipe_path) != 0) {
        fprintf(stderr, "Couldn't setup client in server\n");
        continue;
      }

      Client *new_client = (Client*) malloc(sizeof(struct Client));
      strcpy(new_client->req_pipe_path, req_pipe_path);
      strcpy(new_client->resp_pipe_path, resp_pipe_path);

      if (pthread_mutex_lock(&cond_mutex) != 0) {return 1;}
      push(new_client, q);
      if (pthread_mutex_unlock(&cond_mutex) != 0) {return 1;}

    }
    else{
      if (pthread_mutex_lock(&cond_mutex) != 0) {return 1;}

      if(!empty_q(q)){
        pthread_cond_signal(&empty_q_cond);
      }

      if(active_clients < MAX_SESSION_COUNT){
        pthread_cond_signal(&active_clients_cond);
      }

      if (pthread_mutex_unlock(&cond_mutex) != 0) {return 1;}
    }
  }

  for(int i = 0; i < MAX_SESSION_COUNT; i++){
    if(pthread_join(tid[i], NULL)){
      fprintf(stderr, "Error joining thread\n");
      return 1;
    }
  }

  close(rx);
  unlink(argv[1]);
  free_q(q);

  ems_terminate();
}