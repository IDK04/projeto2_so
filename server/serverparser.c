#include "serverparser.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "common/constants.h"
#include "common/io.h"

enum Request get_next_req(int fd) {
  char buf[3];
  if (read(fd, buf, 2) != 2) {
    return EOC;
  }

  switch (buf[0]) {
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

int parse_str(int fd, char *str){
  char buf[1];
  int i = 0;

  while (1) {
    ssize_t read_bytes = read(fd, buf, 1);
    if (read_bytes == -1) {
      return 1;
    } else if (read_bytes == 0) {
      break;
    }
    
    if(buf[0] == '\n' || buf[0] == '|')
      break;

    str[i] = buf[0];
    i++;
  }

  str[i] = '\0';

  return 0;
}


