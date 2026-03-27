#ifndef DATA_IO_MODULE_H
#define DATA_IO_MODULE_H

#include "data_definitions.h"
#include "http_definitions.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * @brief Blocking, non-asynchronous for now.
 */
struct DataIO {};

/*
 * @brief Initialises a DataIO module. Currently a no-op.
 *
 * @param Caller-created DataIO object.
 */
void init_data_io(struct DataIO *data_io_module) {}

/*
 * @brief Validates a uri, whether it is allowed to be accessed by the public.
 *
 * @param uri Must be a null-terminated string.
 */
bool validate_uri(char uri[]) {
  // TODO:
  // Should only allow relative path, and in current directory
  // Should only allow txt files

  // TODO: simple restriction for now
  const char allowed_file[] = "hello.txt";
  return strncmp(uri, allowed_file, G_MAX_URI_LEN) == 0;
}

/*
 * @brief Gets the file data from file system and puts it into data_buffer.
 *
 * @param data_buffer Ensure this has the right size G_MAX_FILE_READ_SIZE.
 * @param request_uri Must be a null-terminated string.
 *
 * @returns Number of bytes that were succesfully read from file. Returns -1 if
 * error.
 */
size_t get_file_data(char data_buffer[], char request_uri[]) {
  if (!validate_uri(request_uri)) {
    fprintf(stderr, "Invalid request URI: unathorized uri access.\n");
    return -1;
  }
   
  char relative_file_path[] = "allowed_files/";
  strcat(relative_file_path, request_uri);

  FILE *file_handle = fopen(relative_file_path, "r");
  if (file_handle == NULL) {
    fprintf(stderr, "Invalid request URI: file not found.\n");
    return -1;
  }

  char file_buffer[G_MAX_FILE_READ_SIZE];

  // TODO: check fstat to warn user that file being read is too large.

  size_t num_bytes = fread(file_buffer, 1, sizeof(file_buffer), file_handle);
  printf("Read %lu bytes during file read.\n", num_bytes);

  // TODO: any other checks here?
  memcpy(data_buffer, file_buffer, num_bytes);
  return num_bytes;
}

#endif
