#include "../lib/logger.h"
#include "testing_utils.h"
#include <string.h>
#include <stdlib.h>

#define STRESS_TEST_LOG_ENTRIES 1000


void test_logger_too_long_filename() {
  printf("Running test_logger_too_long_filename.\n");

  char too_long_filename[LOG_FILENAME_MAX_LENGTH + 1];  // 1 character too long
  memset(too_long_filename, 'a', LOG_FILENAME_MAX_LENGTH);
  too_long_filename[LOG_FILENAME_MAX_LENGTH] = '\0';
  struct Logger* logger = malloc(sizeof(struct Logger));
  int success = create_logger(logger, INFO, true, true, too_long_filename);
  // logger should return success but fall back to default timestamp-based filename generation.
  assert (success == 0 && strstr(logger->log_filename, "log_") != NULL); 
  remove_test_file(logger->log_filename);
  free_logger(logger);

  print_test_passed();
}


void test_logger_invalid_filename() {
  printf("Running test_logger_invalid_filename.\n");

  char invalid_filename[] = "Invalid??.log";  // invalid characters in filename (e.g. ?)
  struct Logger* logger = malloc(sizeof(struct Logger));
  int success = create_logger(logger, INFO, true, true, invalid_filename);
  if (success == 0) remove_test_file(logger->log_filename); // remove the file if it somehow got created despite the invalid name.
  assert (success == -1); 
  free_logger(logger);
  
  print_test_passed();
}


void test_logger_no_output() {
  printf("Running test_logger_no_output.\n");

  struct Logger* logger = malloc(sizeof(struct Logger));
  int success = create_logger(logger, INFO, false, false, NULL);
  // logger should return an error code due to misconfiguration.
  assert (success == -1); 
  free_logger(logger);
  
  print_test_passed();
}


void test_logger_info() {
  printf("Running test_logger_info.\n");

  char logger_filename[] = "test_logger_level_info.log";
  struct Logger* logger_info = malloc(sizeof(struct Logger));
  int success = create_logger(logger_info, INFO, true, true, logger_filename);
  assert(success == 0);
  assert(log_info(logger_info, "This is an info message that should appear in both loggers.") == 0);
  assert(log_debug(logger_info, "This is a debug message that should NOT appear in logger_info.") == 0);

  FILE* file_info = fopen(logger_filename, "r");
  assert(file_info != NULL);
  char buffer[LOG_MESSAGE_MAX_LENGTH + LOG_TIMESTAMP_FORMAT_MAX_LENGTH];
  
  // logger_info should log only INFO message.
  fgets(buffer, sizeof(buffer), file_info);
  assert(buffer != NULL && strstr(buffer, "This is an info message") != NULL);
  assert(fgets(buffer, sizeof(buffer), file_info) == NULL); // No more log entries should be present.
  fclose(file_info);
  remove_test_file(logger_filename);
  free_logger(logger_info);

  print_test_passed();
}

void test_logger_debug() {
  printf("Running test_logger_debug.\n");

  char logger_filename[] = "test_logger_level_debug.log";
  struct Logger* logger_debug = malloc(sizeof(struct Logger));
  int success = create_logger(logger_debug, DEBUG, true, true, logger_filename);
  assert(success == 0);
  assert(log_info(logger_debug, "This is an info message that should appear in both loggers.") == 0);
  assert(log_debug(logger_debug, "This is a debug message that should ONLY appear in logger_debug.") == 0);

  // logger_debug should log both INFO and DEBUG messages.
  FILE * file_debug = fopen(logger_filename, "r");
  assert(file_debug != NULL);
  char buffer[LOG_MESSAGE_MAX_LENGTH + LOG_TIMESTAMP_FORMAT_MAX_LENGTH];

  fgets(buffer, sizeof(buffer), file_debug);
  assert(buffer != NULL && strstr(buffer, "This is an info message") != NULL);
  fgets(buffer, sizeof(buffer), file_debug);
  assert(buffer != NULL && strstr(buffer, "This is a debug message") != NULL);
  assert(fgets(buffer, sizeof(buffer), file_debug) == NULL); // No more log entries should be present.
  fclose(file_debug);
  remove_test_file(logger_filename);
  free_logger(logger_debug);

  print_test_passed();
}


void test_logger_stress(bool print_to_console) {
  printf("Running test_logger_stress.\n");

  char logger_filename[] = "test_logger_stress.log";
  struct Logger* logger = malloc(sizeof(struct Logger));
  int success = create_logger(logger, DEBUG, print_to_console, true, logger_filename);
  assert(success == 0);
  clock_t start_time = clock();

  for (int i = 0; i < STRESS_TEST_LOG_ENTRIES; ++i) {
    char message[LOG_MESSAGE_MAX_LENGTH];
    snprintf(message, 
             sizeof(message), 
             "STRESS TEST: Log entry number %d out of %d", 
             i + 1, STRESS_TEST_LOG_ENTRIES);
    log_info(logger, message);
  }
  clock_t end_time = clock();
  double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  printf("Completed logger stress time. Took %.2f ms to print %d log entries.\n", 
         elapsed_time * 1000 , STRESS_TEST_LOG_ENTRIES);
  remove_test_file(logger_filename);
  free_logger(logger);

  print_test_passed();
}


int main() {
  test_logger_too_long_filename();
  test_logger_invalid_filename();
  test_logger_no_output();
  test_logger_info();
  test_logger_debug();
  test_logger_stress(false);
  return 0;
}