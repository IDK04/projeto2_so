#ifndef SERVER_PARSER_H
#define SERVER_PARSER_H

#include <stddef.h>
#include "common/constants.h"

enum Request
{
  SETUP,
  QUIT,
  CREATE,
  RESERVE,
  SHOW,
  LIST_EVENTS,
  EOC // End of requests
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

/// Parses a CREATE message.
/// @param fd File descriptor to read from.
/// @param event_id Int to store the event_id.
/// @param num_rows Int to store the number of rows.
/// @param num_cols Int to store the number of columns.
/// @param session_id Int to store the id.
/// @return 0 if the request was parsed successfully, 1 otherwise.
int parse_create(int fd, unsigned int *event_id, size_t *num_rows, size_t *num_cols, int *session_id);

/// Parses a CREATE message.
/// @param fd File descriptor to read from.
/// @param event_id to store the event_id.
/// @param num_seats to store the number of seats.
/// @param xs to store the X´s coordinates
/// @param ys Int to store Y´s coordinates.
/// @return 0 if the request was parsed successfully, 1 otherwise.
int parse_reserve(int fd,unsigned int* event_id, size_t *num_seats,size_t *xs,size_t *ys,int *session_id);

/// Parses a session id.
/// @param fd File descriptor to read from.
/// @param session_id Int to store the id.
/// @return 0 if the request was parsed successfully, 1 otherwise.
int parse_session_id(int fd, int *session_id);

/// Parses a session id.
/// @param fd File descriptor to read from.
/// @param session_id Int to store the id.
/// @param event_id to store the event_id.
/// @return 0 if the request was parsed successfully, 1 otherwise.
int parse_show(int fd,unsigned int* event_id, int *session_id);

/// Parses a string.
/// @param fd File descriptor to read from.
/// @param str String to store in.
/// @return 0 if the string was parsed successfully, 1 otherwise.
int parse_str(int fd, char *str);

#endif // SERVER_PARSER_H
