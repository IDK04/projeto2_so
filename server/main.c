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

void *clientThread(void *arguments){
  int *parsed_arguments = (int*) arguments;


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

  pthread_t tid[1];

  for(int i = 0; i < 1; i++){
    if(pthread_create(&tid[i], 0, clientThread, &i) != 0){
      fprintf(stderr, "Error creating thread\n");
      return 1;
    }
  }

  int kill_server = 0;

  while (1) {

    char req_pipe_path[40], resp_pipe_path[40];
    enum Request req = get_next_req(rx); // will either be SETUP or EOC
    if (req == SETUP){
      if (parse_setup(rx, req_pipe_path, resp_pipe_path) != 0) {
        fprintf(stderr, "Couldn't setup client in server\n");
        continue;
      }
      
      printf("%s-%s\n",req_pipe_path, resp_pipe_path);
      kill_server = 0;

      // ADD TO QUEUE

    }
    else{

      if(!kill_server){
        sleep(2);
        kill_server = 1;
        continue;
      }

      break;
    }
  }

  for(int i = 0; i < 1; i++){
    if(pthread_join(tid[i], NULL)){
      fprintf(stderr, "Error joining thread\n");
      return 1;
    }
  }

  //TODO: Close Server
  close(rx);
  unlink(argv[1]);

  ems_terminate();
}