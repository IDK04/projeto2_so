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

int req_fd=-1, resp_fd=-1;
char const *_req_pipe_path, *_resp_pipe_path;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path, int *session_id) {

  unlink(req_pipe_path);
  unlink(resp_pipe_path);

  _req_pipe_path = req_pipe_path;
  _resp_pipe_path = resp_pipe_path;

  if(mkfifo(req_pipe_path,0777)<0)
    return 1;
  if(mkfifo(resp_pipe_path,0777)<0)
    return 1;
  
  int server_fd = open(server_pipe_path, O_WRONLY);
  if(server_fd < 0)
    return 1;

  // Protocol: OP_CODE|REQ_PIPE_PATH|RESP_PIPE_PATH
  char request[sizeof(char)+(2*sizeof(char)*MAX_PIPE_PATH_SIZE)];
  memset(request, '\0', sizeof(request));
  char op_code = '1';
  memcpy(request, &op_code, sizeof(char));
  strncpy(request+sizeof(char), req_pipe_path, sizeof(char)*MAX_PIPE_PATH_SIZE);
  strncpy(request+sizeof(char)+sizeof(char)*MAX_PIPE_PATH_SIZE, resp_pipe_path, sizeof(char)*MAX_PIPE_PATH_SIZE);

  ssize_t bytes_written = write(server_fd, request, sizeof(request));
  if (bytes_written < 0)
    return 1;
  
  close(server_fd);

  // Get the response from the server (this session id)
  resp_fd = open(resp_pipe_path, O_RDONLY);
  if(resp_fd < 0)
    return 1;

  if (read(resp_fd, session_id, sizeof(int)) < 0) {
    return 1;
  }

  // to be open all the time
  req_fd = open(req_pipe_path, O_WRONLY);
  if(req_fd < 0)
    return 1;

  return 0;
}

int ems_quit(int session_id) {
  
  char request[sizeof(char)+sizeof(int)];
  memset(request, '\0', sizeof(request));
  char op_code = '2';
  memcpy(request, &op_code, sizeof(char));
  memcpy(request+sizeof(char), &session_id, sizeof(int));

  ssize_t bytes_written = write(req_fd, request, sizeof(request));
  if (bytes_written < 0)
    return 1;
  
  close(req_fd);
  close(resp_fd);
  unlink(_req_pipe_path);
  unlink(_resp_pipe_path);

  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols, int *session_id) {
  // Protocol: OP_CODE|REQ_PIPE_PATH|RESP_PIPE_PATH
  char request[sizeof(char)+sizeof(int)+sizeof(unsigned int)+2*sizeof(size_t)];
  memset(request, '\0', sizeof(request));
  char op_code = '3';
  memcpy(request, &op_code, sizeof(char));
  memcpy(request+sizeof(char), session_id, sizeof(int));
  memcpy(request+sizeof(char)+sizeof(int), &event_id, sizeof(unsigned int));
  memcpy(request+sizeof(char)+sizeof(int)+sizeof(unsigned int),&num_rows,sizeof(size_t));
  memcpy(request+sizeof(char)+sizeof(int)+sizeof(unsigned int)+sizeof(size_t),&num_cols,sizeof(size_t));

  ssize_t bytes_written = write(req_fd, request, sizeof(request));
  if (bytes_written < 0)
    return 1;

  // Wait for the response
  int return_value=-1;
  if (read(resp_fd,&return_value, sizeof(int)) < 0) {
    return 1;
  }
  // server Error case
  if(return_value){
    return 1;
  }
  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys,int *session_id) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  char request[sizeof(char)+sizeof(int)+sizeof(unsigned int)+sizeof(size_t)+2*sizeof(size_t)*num_seats];
  memset(request, '\0', sizeof(request));
  char op_code = '4';
  memcpy(request, &op_code, sizeof(char));
  memcpy(request+sizeof(char), session_id, sizeof(int));
  memcpy(request+sizeof(char)+sizeof(int), &event_id, sizeof(unsigned int));
  memcpy(request+sizeof(char)+sizeof(int)+sizeof(unsigned int),&num_seats,sizeof(size_t));
  memcpy(request+sizeof(char)+sizeof(int)+sizeof(unsigned int)+sizeof(size_t),xs,num_seats*sizeof(size_t));
  memcpy(request+sizeof(char)+sizeof(int)+sizeof(unsigned int)+sizeof(size_t)+num_seats*sizeof(size_t),ys,num_seats*sizeof(size_t));

  ssize_t bytes_written = write(req_fd, request, sizeof(request));
  if (bytes_written < 0)
    return 1;

  // Wait for the response
  int return_value=-1;
  if(read(resp_fd,&return_value, sizeof(int)) < 0){
    return 1;
  }
  // server Error case
  if(return_value){
    return 1;
  }
  return 0;
}

int ems_show(int out_fd, unsigned int event_id,int* session_id) {
  char request[sizeof(char)+sizeof(int)+sizeof(unsigned int)];
  memset(request, '\0', sizeof(request));
  char op_code = '5';
  memcpy(request, &op_code, sizeof(char));
  memcpy(request+sizeof(char), session_id, sizeof(int));
  memcpy(request+sizeof(char)+sizeof(int), &event_id, sizeof(unsigned int));

  ssize_t bytes_written = write(req_fd, request, sizeof(request));
  if (bytes_written < 0)
    return 1;

  // Wait for the response
  int return_value=-1;
  if(read(resp_fd,&return_value, sizeof(int)) < 0){
    return 1;
  }
  // server Error case
  if(return_value){
    return 1;
  }
  size_t num_cols,num_rows;
  if (read(resp_fd,&num_cols, sizeof(size_t)) < 0){return 1;}
  if (read(resp_fd,&num_rows, sizeof(size_t)) < 0){return 1;}
  unsigned int seats[num_cols*num_rows];
  if (read(resp_fd,&seats, sizeof(seats)) < 0){return 1;}

  for (size_t i = 0; i < num_rows; i++) {
    for (size_t j = 0; j < num_cols; j++) {
      char buffer[16];
      sprintf(buffer, "%u", seats[i*num_cols+j]);

      if (print_str(out_fd, buffer)) {
        perror("Error writing to file descriptor");
        return 1;
      }

      if (j < num_cols) {
        if (print_str(out_fd, " ")) {
          perror("Error writing to file descriptor");
          return 1;
        }
      }
    }

    if (print_str(out_fd, "\n")) {
      perror("Error writing to file descriptor");
      return 1;
    }
  }
  return 0;
}

int ems_list_events(int out_fd,int *session_id) {
  char request[sizeof(char)+sizeof(int)+sizeof(unsigned int)];
  memset(request, '\0', sizeof(request));
  char op_code = '6';
  memcpy(request, &op_code, sizeof(char));
  memcpy(request+sizeof(char), session_id, sizeof(int));

  ssize_t bytes_written = write(req_fd, request, sizeof(request));
  if (bytes_written < 0)
    return 1;

  int return_value=-1;
  if(read(resp_fd,&return_value, sizeof(int)) < 0) {
    return 1;
  }
  // server Error case
  if(return_value){
    return 1;
  }
  size_t num_events;
  if (read(resp_fd,&num_events, sizeof(size_t)) < 0){return 1;}
  unsigned int ids[num_events];
  if (read(resp_fd,ids, sizeof(ids)) < 0){return 1;}

  if (num_events == 0) {
    char buff[] = "No events\n";
    if (print_str(out_fd, buff)) {
      perror("Error writing to file descriptor");
      return 1;
    }
    return 0;
  }

  for(size_t i = 0; i < num_events; i++) {
    char buff[] = "Event: ";
    if (print_str(out_fd, buff)) {
      perror("Error writing to file descriptor");
      return 1;
    }

    char id[16];
    sprintf(id, "%u\n", ids[i]);
    if (print_str(out_fd, id)) {
      perror("Error writing to file descriptor");
      return 1;
    }
  }

  return 0;
}
