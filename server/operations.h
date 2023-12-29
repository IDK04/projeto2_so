#ifndef SERVER_OPERATIONS_H
#define SERVER_OPERATIONS_H

#include <stddef.h>

/// Initializes the EMS state.
/// @param delay_us Delay in microseconds.
/// @return 0 if the EMS state was initialized successfully, 1 otherwise.
int ems_init(unsigned int delay_us);

/// Destroys the EMS state.
int ems_terminate();

/// Creates a new event with the given id and dimensions.
/// @param event_id Id of the event to be created.
/// @param num_rows Number of rows of the event to be created.
/// @param num_cols Number of columns of the event to be created.
/// @return 0 if the event was created successfully, 1 otherwise.
int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols);

/// Creates a new reservation for the given event.
/// @param event_id Id of the event to create a reservation for.
/// @param num_seats Number of seats to reserve.
/// @param xs Array of rows of the seats to reserve.
/// @param ys Array of columns of the seats to reserve.
/// @return 0 if the reservation was created successfully, 1 otherwise.
int ems_reserve(unsigned int event_id, size_t num_seats, size_t *xs, size_t *ys);

/// Prints every event.
/// @param event_id Id of the event to be shown.
/// @param num_cols Number of columns.
/// @param num_rows Number of rows.
/// @param steats Seats to print.
/// @return 0 if the event was printed successfully, 1 otherwise.
int ems_show_events_server(unsigned int event_id, size_t num_rows, size_t num_cols, unsigned int *seats);

/// Prints the given event.
/// @param event_id Id of the event to be shown.
/// @param num_cols Number of columns.
/// @param num_rows Number of rows.
/// @return 0 if the event was printed successfully, 1 otherwise.
int ems_show(char *buffer, unsigned int event_id, size_t *num_cols, size_t *num_rows);

/// Prints all the events.
/// @param out_fd File descriptor to print the events to.
/// @return 0 if the events were printed successfully, 1 otherwise.
int ems_list_events(char *buffer, size_t *num_events);

#endif  // SERVER_OPERATIONS_H
