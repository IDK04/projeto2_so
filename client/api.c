#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "api.h"
#include "common/io.h"
#include "common/constants.h"

char* build_message(const char **arguments, int num_arguments){
  char* result = malloc(sizeof(char)*MAX_REQUEST_SIZE);

  if(!arguments)
    return NULL;

  strcat(result,arguments[0]);

  for (int i = 1; i < num_arguments; i++){
    strcat(result,"|");
    strcat(result,arguments[i]);
  }

  strcat(result,"\n");

  return result;
}

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path, int *session_id) {

  unlink(req_pipe_path);
  unlink(resp_pipe_path);

  if(mkfifo(req_pipe_path,0777)<0)
    return 1;
  if(mkfifo(resp_pipe_path,0777)<0)
    return 1;
  
  int server_fd = open(server_pipe_path, O_WRONLY);
  if(server_fd < 0)
    return 1;

  // Protocol: OP_CODE|REQ_PIPE_PATH|RESP_PIPE_PATH
  const char* arguments[3];
  arguments[0] = "1";
  arguments[1] = req_pipe_path;
  arguments[2] = resp_pipe_path;
  char * msg = build_message(arguments, 3);

  ssize_t bytes_written = write(server_fd, msg, strlen(msg));
  if (bytes_written < 0)
    return 1;

  free(msg);

  // Get the response from the server (this session id)
  /*
  int resp_fd = open(resp_pipe_path, O_RDONLY);
  if(resp_fd < 0)
    return 1;
  
  if (parse_uint(resp_fd, session_id, NULL)){
    return 1;
  }*/

  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_list_events(int out_fd) {
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}
