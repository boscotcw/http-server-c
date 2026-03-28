#include "../lib/logger.h"
#include "testing_utils.h"
#include <string.h>

#define STRESS_TEST_LOG_ENTRIES 1000


// TODO: Add more modularized tests for logger, including exception handling for overly long log messages, illegal filenames, and multiple loggers writing to the same log file. 
void test_logger_illegal_filename() {
  // WIP: add exception handling for illegal filenames in create_logger.
  return; // Skipped for now

  // printf("Running test_logger_illegal_filename.\n");

  // char illegal_filename[] = "test_illegal_filename?"; 
  // printf("An error should be triggered during logger creation.\n");
  // struct Logger logger = create_logger(INFO, true, true, illegal_filename);

  // print_test_passed();
}


void test_logger_info() {
  printf("Running test_logger_info.\n");

  char logger_filename[] = "test_logger_level_info.log";
  struct Logger logger_info = create_logger(INFO, true, true, logger_filename);
  log_info(logger_info, "This is an info message that should appear in both loggers.");
  log_debug(logger_info, "This is a debug message that should NOT appear in logger_info.");

  FILE * file_info = fopen(logger_filename, "r");
  assert(file_info != NULL);
  char buffer[LOG_MESSAGE_MAX_LENGTH + LOG_TIMESTAMP_FORMAT_MAX_LENGTH];
  
  // logger_info should log only INFO message.
  fgets(buffer, sizeof(buffer), file_info);
  assert(buffer != NULL && strstr(buffer, "This is an info message") != NULL);
  assert(fgets(buffer, sizeof(buffer), file_info) == NULL); // No more log entries should be present.
  fclose(file_info);
  remove_test_file(logger_filename);

  print_test_passed();
}

void test_logger_debug() {
  printf("Running test_logger_debug.\n");

  char logger_filename[] = "test_logger_level_debug.log";
  struct Logger logger_debug = create_logger(DEBUG, true, true, logger_filename);
  log_info(logger_debug, "This is an info message that should appear in both loggers.");
  log_debug(logger_debug, "This is a debug message that should ONLY appear in logger_debug.");

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

  print_test_passed();
}


void test_logger_stress(bool print_to_console) {
  printf("Running test_logger_stress.\n");

  char logger_filename[] = "test_logger_stress.log";
  struct Logger logger = create_logger(DEBUG, print_to_console, true, logger_filename);
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

  print_test_passed();
}


int main() {
  test_logger_illegal_filename();
  test_logger_info();
  test_logger_debug();
  test_logger_stress(false);
  return 0;
}
