#include "../lib/logger.h"
#include "testing_utils.h"
#include <string.h>
#include <stdlib.h>

#define STRESS_TEST_LOG_ENTRIES 100000

void safe_assert(bool condition, struct Logger* logger) {
  // Use safe_assert if there is a log file to clean up after a failed assertion.
  if (!condition) {
    fprintf(stderr, "Test failed: %s\n", logger->log_filename);
    if (logger && logger->log_filename) {
      int result = fclose(logger->log_file); // Ensure file is closed before removal
      printf("Closing log file before removal: %s\t result: %d\n", logger->log_filename, result);
      if (result != 0) {
        fprintf(stderr, "Error closing log file before removal: %s\n", logger->log_filename);
      }
      remove_test_file(logger->log_filename);
    }
    if (logger->log_filename) remove_test_file(logger->log_filename);
    assert(condition);
  }
}


void logger_test_cleanup(struct Logger* logger) {
  if (logger->log_file != NULL) {
    // Close the log file if it's still open before further removal attempts.
    fclose(logger->log_file);
    logger->log_file = NULL;
  }
  if (logger->generate_log_file && logger->log_filename != NULL) {
    printf("Cleaning up log file: %s\n", logger->log_filename);
    remove_test_file(logger->log_filename);
  }
  free_logger(logger);
}


void test_logger_too_long_filename() {
  printf("Running test_logger_too_long_filename.\n");

  char too_long_filename[LOG_FILENAME_MAX_LENGTH + 1];  // 1 character too long
  memset(too_long_filename, 'a', LOG_FILENAME_MAX_LENGTH);
  too_long_filename[LOG_FILENAME_MAX_LENGTH] = '\0';
  struct Logger* logger = malloc(sizeof(struct Logger));
  int success = create_logger(logger, INFO, true, true, true, too_long_filename);
  // logger should return success but use fallback timestamp filename.
  safe_assert(success == 0 && strstr(logger->log_filename, "log_") != NULL, logger); 
  logger_test_cleanup(logger);

  print_test_passed();
}


void test_logger_invalid_filename() {
  printf("Running test_logger_invalid_filename.\n");

  char invalid_filename[] = "Invalid??.log";  // invalid characters in filename (e.g. ?)
  struct Logger* logger = malloc(sizeof(struct Logger));
  int success = create_logger(logger, INFO, true, true, true, invalid_filename);
  // logger should return success but use fallback timestamp filename.
  safe_assert(success == 0 && strstr(logger->log_filename, "log_") != NULL, logger); 
  logger_test_cleanup(logger);
  
  print_test_passed();
}


void test_logger_no_output() {
  printf("Running test_logger_no_output.\n");

  struct Logger* logger = malloc(sizeof(struct Logger));
  int success = create_logger(logger, INFO, false, false, true, NULL);
  // logger should return an error code due to misconfiguration.
  safe_assert(success == -1, logger);
  logger_test_cleanup(logger);
  
  print_test_passed();
}


void test_logger_info() {
  printf("Running test_logger_info.\n");

  char logger_filename[] = "test_logger_level_info.log";
  struct Logger* logger_info = malloc(sizeof(struct Logger));
  int success = create_logger(logger_info, INFO, true, true, true, logger_filename);
  safe_assert(success == 0, logger_info);
  safe_assert(log_info(logger_info, "This is an info message that should appear in both loggers.") == 0, logger_info);
  safe_assert(log_debug(logger_info, "This is a debug message that should NOT appear in logger_info.") == 0, logger_info);
  FILE* file_info = fopen(logger_filename, "r");
  safe_assert(file_info != NULL, logger_info);
  char buffer[LOG_MESSAGE_MAX_LENGTH + LOG_TIMESTAMP_FORMAT_MAX_LENGTH];
  // logger_info should log only INFO message.
  fgets(buffer, sizeof(buffer), file_info);
  
  safe_assert(buffer != NULL && strstr(buffer, "This is an info message") != NULL, logger_info);
  safe_assert(fgets(buffer, sizeof(buffer), file_info) == NULL, logger_info); // No more log entries should be present.
  fclose(file_info);
  logger_test_cleanup(logger_info);

  print_test_passed();
}

void test_logger_debug() {
  printf("Running test_logger_debug.\n");

  char logger_filename[] = "test_logger_level_debug.log";
  struct Logger* logger_debug = malloc(sizeof(struct Logger));
  int success = create_logger(logger_debug, DEBUG, true, true, true, logger_filename);
  safe_assert(success == 0, logger_debug);
  safe_assert(log_info(logger_debug, "This is an info message that should appear in both loggers.") == 0, logger_debug);
  safe_assert(log_debug(logger_debug, "This is a debug message that should ONLY appear in logger_debug.") == 0, logger_debug);

  // logger_debug should log both INFO and DEBUG messages.
  FILE * file_debug = fopen(logger_filename, "r");
  safe_assert(file_debug != NULL, logger_debug);
  char buffer[LOG_MESSAGE_MAX_LENGTH + LOG_TIMESTAMP_FORMAT_MAX_LENGTH];

  fgets(buffer, sizeof(buffer), file_debug);
  safe_assert(buffer != NULL && strstr(buffer, "This is an info message") != NULL, logger_debug);
  fgets(buffer, sizeof(buffer), file_debug);
  safe_assert(buffer != NULL && strstr(buffer, "This is a debug message") != NULL, logger_debug);
  safe_assert(fgets(buffer, sizeof(buffer), file_debug) == NULL, logger_debug); // No more log entries should be present.
  fclose(file_debug);
  logger_test_cleanup(logger_debug);

  print_test_passed();
}


void test_logger_stress(bool print_to_console, bool generate_log_file, bool always_flush) {
  printf("Running test_logger_stress. print_to_console: %s, generate_log_file: %s, always_flush: %s\n",
         print_to_console ? "true" : "false",
         generate_log_file ? "true" : "false",
         always_flush ? "true" : "false");

  char logger_filename[] = "test_logger_stress.log";
  struct Logger* logger = malloc(sizeof(struct Logger));
  int success = create_logger(logger, DEBUG, print_to_console, generate_log_file, always_flush, logger_filename);
  safe_assert(success == 0, logger);
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
  logger_test_cleanup(logger);

  print_test_passed();
}


void test_logger_flush() {
  // See if we need a test for this or if we can just verify it works in the stress test.
  return;
}


int main() {
  test_logger_too_long_filename();
  test_logger_invalid_filename();
  test_logger_no_output();
  test_logger_info();
  test_logger_debug();
  test_logger_stress(false, true, true);  // file output only + instant flush
  test_logger_stress(false, true, false);  // file output only + buffering
  // test_logger_stress(true, false, false);  // print output only
  return 0;
}