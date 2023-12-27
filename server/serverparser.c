#include "serverparser.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "common/constants.h"
#include "common/io.h"

enum Request get_next_req(int fd) {
  char op_code;
  ssize_t bytes_read = read(fd, &op_code, sizeof(char));
  if (bytes_read == 0) {
    return QUIT;
  }
  else if(bytes_read < 0){
    return EOC;
  }

  switch (op_code) {
    case '1':
      return SETUP;

    case '2':
      return QUIT;

    case '3':
      return CREATE;

    case '4':
      return RESERVE;

    case '5':
      return SHOW;

    case '6':
      return LIST_EVENTS;
    
    default:
      return EOC;
  }
}

int parse_setup(int fd, char* req_pipe_path, char* resp_pipe_path) {
  
  if (parse_str(fd, req_pipe_path) != 0) {
    return 1;
  }

  if (parse_str(fd, resp_pipe_path) != 0) {
    return 1;
  }

  return 0;
}

int parse_create(int fd, unsigned int *event_id, size_t *num_rows, size_t *num_cols, int *session_id){
  if(parse_session_id(fd, session_id)){
    return 1;
  }

  if (read(fd,event_id, sizeof(unsigned int)) < 0) {
    return 1;
  }
  if (read(fd,num_rows, sizeof(size_t)) < 0) {
    return 1;
  }
  if (read(fd,num_cols, sizeof(size_t)) < 0) {
    return 1;
  }
  return 0;
}

int parse_session_id(int fd, int *session_id){
  if (read(fd, session_id, sizeof(int)) < 0) {
    return 1;
  }
  return 0;
}

int parse_str(int fd, char *str){

  if(read(fd, str, MAX_PIPE_PATH_SIZE)<0){
    return 1;
  }

  return 0;
}

int parse_reserve(int fd,unsigned int* event_id, size_t *num_seats,size_t *xs,size_t *ys,int *session_id){
  if(parse_session_id(fd, session_id)){
    return 1;
  }

  if (read(fd,event_id, sizeof(unsigned int)) < 0) {
    return 1;
  }
  if (read(fd,num_seats, sizeof(size_t)) < 0) {
    return 1;
  }
  if (read(fd,xs, *num_seats*sizeof(size_t)) < 0) {
    return 1;
  }
  if (read(fd,ys, *num_seats*sizeof(size_t)) < 0) {
    return 1;
  }
  return 0;
}

int parse_show(int fd,unsigned int* event_id,int *session_id){
  if(parse_session_id(fd, session_id)){
    return 1;
  }

  if (read(fd,event_id, sizeof(unsigned int)) < 0) {
    return 1;
  }
  return 0;
}
