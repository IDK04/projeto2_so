#include "serverparser.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "common/constants.h"
#include "common/io.h"

enum Request get_next_req(int fd) {
  int op_code;
  if (read(fd, &op_code, sizeof(int)) < 0) {
    return EOC;
  }

  switch (op_code) {
    case 1:
      return SETUP;

    case 2:
      return QUIT;

    case 3:
      return CREATE;

    case 4:
      return RESERVE;

    case 5:
      return SHOW;

    case 6:
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

int parse_str(int fd, char *str){

  if(read(fd, str, MAX_PIPE_PATH_SIZE)<0){
    return 1;
  }

  return 0;
}


