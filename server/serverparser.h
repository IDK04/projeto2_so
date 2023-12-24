#ifndef SERVER_PARSER_H
#define SERVER_PARSER_H

#include <stddef.h>
#include "common/constants.h"

enum Request {
  SETUP,
  QUIT,
  CREATE,
  RESERVE,
  SHOW,
  LIST_EVENTS,
  EOC  // End of requests
};

/// Reads a line and returns the corresponding request.
/// @param fd File descriptor to read from.
/// @return The request read.
enum Request get_next_req(int fd);

/// Parses a SETUP message.
/// @param fd File descriptor to read from.
/// @param req_pipe_path String to store the request pipe of this client.
/// @param resp_pipe_path String to store the response pipe of this client.
/// @return 0 if the request was parsed successfully, 1 otherwise.
int parse_setup(int fd, char *req_pipe_path, char *resp_pipe_path);

/// Parses a string.
/// @param fd File descriptor to read from.
/// @param str String to store in.
/// @return 0 if the string was parsed successfully, 1 otherwise.
int parse_str(int fd, char *str);

#endif  // SERVER_PARSER_H
