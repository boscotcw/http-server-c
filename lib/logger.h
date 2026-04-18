#ifndef LOGGER_H
#define LOGGER_H

// increase limit if needed
#define LOG_FILENAME_MAX_LENGTH 64
#define LOG_MESSAGE_MAX_LENGTH 512
#define LOG_TIMESTAMP_FORMAT_MAX_LENGTH 32
// The timestamp format of "[YYYY-MM-DD HH:MM:SS]\t" plus \n and \0 is 24 characters long. 
// We set the max length to 32 to be safe.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

enum LogLevel {
  DEBUG,
  INFO
};

struct Logger {
  bool print_to_console;
  bool generate_log_file;
  char log_filename[LOG_FILENAME_MAX_LENGTH];
  bool always_flush;
  FILE* log_file;
  enum LogLevel verbosity;
};

int write_log_file(struct Logger* logger, const char log_entry[]) {
  // Writes log entry to a file.
  if (!logger->generate_log_file) return -1; // Double check

  if (logger->log_file == NULL) {
    logger->log_file = fopen(logger->log_filename, "a");
  }
  if (logger->log_file == NULL) {
    fprintf(stderr, "Error: Log file is not available.\n");
    return -1;
  }
  
  fprintf(logger->log_file, "%s", log_entry);
  if (ferror(logger->log_file)) {
    fprintf(stderr, "Error: Unable to write to log file.\n");
    fclose(logger->log_file);
    logger->log_file = NULL;
    return -1;
  }
  
  if (logger->always_flush) {
    int success = fflush(logger->log_file); // Ensure the log entry is written to the file immediately
    if (success != 0) {
      fprintf(stderr, "Error: Failed to flush log file.\n");
      fclose(logger->log_file);
      logger->log_file = NULL;
      return -1;
    }
  }
  return 0;
}

int generate_log(struct Logger* logger, const char message[], enum LogLevel log_level) {
  // Generate & print a log entry with a timestamp & message. 
  // Optionally write the log entry to a file if logger is configured to do so. 
  if (!logger) return -1;
  if (log_level < logger->verbosity) return 0; // Skip log entries below the configured verbosity level
  if (strnlen(message, LOG_MESSAGE_MAX_LENGTH + 1) > LOG_MESSAGE_MAX_LENGTH) {
    fprintf(stderr, "Warning: Log message exceeds maximum length.\n");
    return -1;
  }
  
  time_t now;
  char time_buf[32];
  time(&now);
  struct tm* time_info = localtime(&now);
  strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", time_info);
  char log_entry[LOG_MESSAGE_MAX_LENGTH + LOG_TIMESTAMP_FORMAT_MAX_LENGTH];
  snprintf(log_entry, sizeof(log_entry), "[%s]\t%s\n", time_buf, message);
  if (logger->print_to_console) printf("%s", log_entry);
  if (!logger->generate_log_file) return 0;

  // Write the log entry to a file
  return write_log_file(logger, log_entry);
}

int log_info(struct Logger* logger, const char message[]) {
/*
 * Logs an info-level message.
 * @param logger The logger instance with configured settings.
 * @param message The message to log.
 */
  return generate_log(logger, message, INFO);
}

int log_debug(struct Logger* logger, const char message[]) {
/*
 * Logs an debug-level message.
 * @param logger The logger instance with configured settings.
 * @param message The message to log.
 */
  return generate_log(logger, message, DEBUG);
}

int create_logger(struct Logger* logger, 
                  enum LogLevel verbosity, 
                  bool print_to_console,
                  bool generate_log_file, 
                  bool always_flush,
                  const char* log_filename) {
/*
 * Creates a Logger with the configured verbosity and file-output behavior.
 * @param verbosity Minimum log level to emit (messages with level >= verbosity are logged).
 * @param print_to_console If true, log entries are printed to the console.
 * @param generate_log_file If true, log entries are appended to a .log file.
 * @param always_flush If true and generate_log_file is true, the log file is flushed after every write to ensure logs are written immediately.
 * @param log_filename Filename to use when generate_log_file is true; if NULL, a default timestamp-based name is used.
 */
  logger->generate_log_file = generate_log_file;
  logger->print_to_console = print_to_console;
  logger->verbosity = verbosity;
  logger->always_flush = always_flush;
  logger->log_file = NULL;

  if (generate_log_file) {
    if (log_filename != NULL && strnlen(log_filename, LOG_FILENAME_MAX_LENGTH + 1) >= LOG_FILENAME_MAX_LENGTH) {
      fprintf(stderr, "Warning: Log filename exceeds maximum length. Using default timestamp-based filename instead.\n");
      log_filename = NULL; // Reset to trigger default filename generation (timestamp)
    }
    if (log_filename == NULL) {
      // Set to default log filename with timestamp
      time_t now;
      time(&now);
      struct tm* time_info = localtime(&now);
      strftime(logger->log_filename, sizeof(logger->log_filename), "log_%Y-%m-%d %H%M%S.log", time_info);
    } else {
      snprintf(logger->log_filename, sizeof(logger->log_filename), "%s", log_filename);
    }
    logger->log_file = fopen(logger->log_filename, "a");
    if (logger->log_file == NULL) {
      // Failed log file creation; retry with default timestamp-based filename
      fprintf(stderr, "Error: failed to create log file.\n");
      time_t now;
      time(&now);
      struct tm* time_info = localtime(&now);
      strftime(logger->log_filename, sizeof(logger->log_filename), "log_%Y-%m-%d %H%M%S.log", time_info);
      logger->log_file = fopen(logger->log_filename, "a");
      if (logger->log_file == NULL) {
        fprintf(stderr, "Error: failed to create log file with default filename. File output will now be disabled.\n");
        logger->generate_log_file = false; // Disable file output if we can't create a log file
      }
    }
  }
  if (!logger->print_to_console && !logger->generate_log_file) {
    fprintf(stderr, "Warning: Logger is configured to neither print to console nor generate log file. No logs will be emitted.\n");
    return -1;  // Return an error code to indicate misconfiguration
  }
  if (always_flush && logger->generate_log_file && logger->log_file) {
    fclose(logger->log_file);
    logger->log_file = NULL;
  }  
  return 0;
}

int free_logger(struct Logger* logger) {
  // Free any resources associated with the logger if needed.
  if (logger->log_file) {
    fclose(logger->log_file); // Close log file if it's still open
    logger->log_file = NULL;
  }
  if (logger) {
    free(logger);
  }
  return 0;
}



#endif
