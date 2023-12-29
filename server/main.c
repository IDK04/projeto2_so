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
#include <signal.h>
#include <stdatomic.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "serverparser.h"
#include "queue.h"

#define MAX_SHOW_SIZE 15000 * sizeof(unsigned int)
#define MAX_LIST_EVENTS_SIZE 1000 * sizeof(unsigned int)

atomic_int kill_server = ATOMIC_VAR_INIT(0);
int toggle_show = 0;

Queue *q;
int active_clients = 0;

pthread_cond_t active_clients_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty_q_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;

int process_request(int req_fd, int resp_fd){
  int session_id_client,res;
  unsigned int event_id;
  size_t num_rows, num_cols, num_seats, xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

  switch (get_next_req(req_fd)){

    case SETUP: //Will never be SETUP
      break;
      
    case QUIT:
      if (parse_session_id(req_fd, &session_id_client)){
        return 1;
      }
      return 1;
    
    case CREATE:
      if(parse_create(req_fd, &event_id, &num_rows, &num_cols, &session_id_client)){
        return 0;
      }
      res = ems_create(event_id,num_rows,num_cols);
      if (write(resp_fd, &res, sizeof(int)) < 0)
        return 0;

      break;

    case RESERVE:
      if(parse_reserve(req_fd, &event_id, &num_seats,xs,ys,&session_id_client)){
        return 0;
      }
      res = ems_reserve(event_id, num_seats, xs, ys);
      if (write(resp_fd, &res, sizeof(int)) < 0)
        return 0;

      break;

    case SHOW:{
      if(parse_show(req_fd,&event_id,&session_id_client)){
          return 0;
      }

      char show_buffer[MAX_SHOW_SIZE];
      res = ems_show(show_buffer, event_id, &num_cols, &num_rows);

      if(res){
        if (write(resp_fd, &res, sizeof(int)) < 0)
          return 0;
      }
      else{
        char response[sizeof(int)+2*sizeof(size_t)+(num_cols*num_rows*sizeof(unsigned int))];
        memcpy(response, &res, sizeof(int));
        memcpy(response+sizeof(int), &num_rows, sizeof(size_t));
        memcpy(response+sizeof(int)+sizeof(size_t), &num_cols, sizeof(size_t));
        memcpy(response+sizeof(int)+(2*sizeof(size_t)), show_buffer, num_cols*num_rows*sizeof(unsigned int));
        if (write(resp_fd, response, sizeof(response)) < 0)
          return 0;
      }

      break;
    }
    case LIST_EVENTS:{
      if (parse_session_id(req_fd, &session_id_client)){
        return 0;
      }
      size_t num_events;

      char list_buffer[MAX_LIST_EVENTS_SIZE];
      res = ems_list_events(list_buffer, &num_events);

      if(res){
        if (write(resp_fd, &res, sizeof(int)) < 0)
          return 0;
      }
      else{
        char response[sizeof(int)+sizeof(size_t)+(num_events*sizeof(unsigned int))];
        memcpy(response, &res, sizeof(int));
        memcpy(response+sizeof(int), &num_events, sizeof(size_t));
        memcpy(response+sizeof(int)+sizeof(size_t), list_buffer, num_events*sizeof(unsigned int));
        if (write(resp_fd, response, sizeof(response)) < 0)
          return 0;
      }
      break;
    }
    case EOC:
      break;
    
    default:
      break;
  }
  return 0;
}

void *clientThread(void *arguments){

  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGUSR1);
  sigaddset(&signal_set, SIGUSR2);

  if(pthread_sigmask(SIG_BLOCK, &signal_set, NULL)!=0){
    printf("Error creating sigmask\n");
    free(arguments);
    return NULL;
  }

  int *parsed_arguments = (int*) arguments;
  int session_id = *parsed_arguments;
  free(parsed_arguments);

  while(1){
    if (pthread_mutex_lock(&cond_mutex) != 0) {return NULL;}

    while (empty_q(q) && !atomic_load(&kill_server)) {
      pthread_cond_wait(&empty_q_cond, &cond_mutex);
    }

    while(active_clients>=MAX_SESSION_COUNT && !atomic_load(&kill_server)){
      pthread_cond_wait(&active_clients_cond, &cond_mutex);
    }

    if(atomic_load(&kill_server)){
      pthread_cond_signal(&empty_q_cond);
      pthread_cond_signal(&active_clients_cond);
      if (pthread_mutex_unlock(&cond_mutex) != 0) {return NULL;}
      break;
    }

    active_clients++;
    Client *client = pop(q);
    if (pthread_mutex_unlock(&cond_mutex) != 0) {return NULL;}

    if (!client)
      return NULL;

    int resp_fd = open(client->resp_pipe_path, O_WRONLY);
    if(resp_fd < 0){
      free(client);
      return NULL;
    }

    if (write(resp_fd, &session_id, sizeof(int)) < 0) {
      free(client);
      return NULL;
    }

    int req_fd = open(client->req_pipe_path, O_RDONLY);
    if(resp_fd < 0){
      free(client);
      return NULL;
    }

    while(!process_request(req_fd, resp_fd));

    close(req_fd);
    close(resp_fd);    
    free(client);

    if (pthread_mutex_lock(&cond_mutex) != 0) {return NULL;}
    active_clients--;
    if (pthread_mutex_unlock(&cond_mutex) != 0) {return NULL;}
  }

  if(pthread_sigmask(SIG_UNBLOCK, &signal_set, NULL)!=0){
    printf("Error unblocking sigmask\n");
    return NULL;
  }

  return NULL;
}

static void kill_server_handler(int sign) {

  if (sign == SIGUSR2)
    atomic_store(&kill_server, 1);

}

static void show_events_handler (int sign) {

  if (sign == SIGUSR1)
    toggle_show = 1;

}

int main(int argc, char* argv[]) {

  if (signal(SIGUSR1, show_events_handler) == SIG_ERR) {
    return 1;
  }

  if (signal(SIGUSR2, kill_server_handler) == SIG_ERR) {
    return 1;
  }

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


  pthread_t tid[MAX_SESSION_COUNT];

  for(int i = 0; i < MAX_SESSION_COUNT; i++){
    int *id = (int*) malloc(sizeof(int));
    *id = i;
    if(pthread_create(&tid[i], 0, clientThread, id) != 0){
      fprintf(stderr, "Error creating thread\n");
      return 1;
    }
  }

  int rx = open(argv[1], O_RDONLY);
  if (rx == -1) {
      fprintf(stderr, "Server open failed: %s\n", strerror(errno));
      return 1;
  }

  while (!atomic_load(&kill_server)) {

    if(toggle_show){
      toggle_show = 0;
      char list_buffer[MAX_LIST_EVENTS_SIZE];
      size_t num_events = 0, num_cols = 0, num_rows = 0;
      int res = ems_list_events(list_buffer, &num_events);
      if(res){
        return 1;
      }
      else{

        if(num_events == 0){
          char buff[] = "No events\n";
          if (print_str(STDOUT_FILENO, buff)) {
            perror("Error writing to file descriptor");
            return 1;
          }
        }

        unsigned int ids[num_events];
        memcpy(ids, list_buffer, sizeof(ids));

        for (size_t i = 0; i < num_events; i++) {
          char show_buffer[MAX_SHOW_SIZE];
          res = ems_show(show_buffer, ids[i], &num_cols, &num_rows);
          if(res){
            return 1;
          }
          else{
            unsigned int seats[num_cols * num_rows];
            memcpy(seats, show_buffer, sizeof(seats));
            if (ems_show_events_server(ids[i], num_rows, num_cols, seats)){
              return 1;
            }
          }
        }
      }
    }

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

  if (pthread_mutex_lock(&cond_mutex) != 0) {return 1;}
  pthread_cond_signal(&empty_q_cond);
  pthread_cond_signal(&active_clients_cond);
  if (pthread_mutex_unlock(&cond_mutex) != 0) {return 1;}

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