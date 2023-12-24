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
  char request[sizeof(int)+(2*sizeof(char)*MAX_PIPE_PATH_SIZE)];
  int op_code = 1;
  memcpy(request, &op_code, sizeof(int));
  strncpy(request+sizeof(int), req_pipe_path, sizeof(char)*MAX_PIPE_PATH_SIZE);
  strncpy(request+sizeof(int)+sizeof(char)*MAX_PIPE_PATH_SIZE, resp_pipe_path, sizeof(char)*MAX_PIPE_PATH_SIZE);

  ssize_t bytes_written = write(server_fd, request, sizeof(request));
  if (bytes_written < 0)
    return 1;

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
