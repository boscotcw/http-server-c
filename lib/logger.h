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
#include <time.h>
#include <string.h>

enum LogLevel {
/*
 * Logs an info-level message.
 * @param logger The logger instance with configured settings.
 * @param message The message to log.
 */
  DEBUG,
  INFO
};

struct Logger {
  bool print_to_console;
  bool generate_log_file;
  char log_filename[LOG_FILENAME_MAX_LENGTH];
  enum LogLevel verbosity;
};

void write_log_file(struct Logger logger, const char log_entry[]) {
  // Writes log entry to a file.
  if (!logger.generate_log_file) return; // Double check if log file generation is enabled

  // The slow way: Open the file, write the log entry, and close the file for each message.
  FILE* log_file = fopen(logger.log_filename, "a");
  if (log_file == NULL) {
    fprintf(stderr, "Error: Log file is not available.\n");
    return;
  }
  fprintf(log_file, "%s", log_entry);
  if (ferror(log_file)) {
    fprintf(stderr, "Error: Unable to write to log file.\n");
    fclose(log_file);
    return;
  }
  fflush(log_file); // Ensure the log entry is written to the file immediately
  fclose(log_file);
}

void generate_log(struct Logger logger, const char message[], enum LogLevel log_level) {
  // Generate & print a log entry with a timestamp & message. 
  // Optionally write the log entry to a file if logger is configured to do so.
  if (log_level < logger.verbosity) return; 
  if (strlen(message) > LOG_MESSAGE_MAX_LENGTH) {
    fprintf(stderr, "Warning: Log message exceeds maximum length.\n");
  }
  
  time_t now;
  char time_buf[32];
  time(&now);
  struct tm* time_info = localtime(&now);
  strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", time_info);
  char log_entry[LOG_MESSAGE_MAX_LENGTH + LOG_TIMESTAMP_FORMAT_MAX_LENGTH];
  snprintf(log_entry, sizeof(log_entry), "[%s]\t%s\n", time_buf, message);
  if (logger.print_to_console) printf("%s", log_entry);

  if (!logger.generate_log_file) return;
  // Write the log entry to a file
  write_log_file(logger, log_entry);
}

void log_info(struct Logger logger, const char message[]) {
/*
 * Logs an info-level message.
 * @param logger The logger instance with configured settings.
 * @param message The message to log.
 */
  generate_log(logger, message, INFO);
}

void log_debug(struct Logger logger, const char message[]) {
/*
 * Logs an debug-level message.
 * @param logger The logger instance with configured settings.
 * @param message The message to log.
 */
  generate_log(logger, message, DEBUG);
}

struct Logger create_logger(enum LogLevel verbosity, 
                            bool print_to_console,
                            bool generate_log_file, 
                            const char* log_filename) {
/*
 * Creates a Logger with the configured verbosity and file-output behavior.
 * @param verbosity Minimum log level to emit (messages with level >= verbosity are logged).
 * @param print_to_console If true, log entries are printed to the console.
 * @param generate_log_file If true, log entries are appended to a .log file.
 * @param log_filename Filename to use when generate_log_file is true; if NULL, a default timestamp-based name is used.
 */
  struct Logger logger;
  logger.generate_log_file = generate_log_file;
  logger.print_to_console = print_to_console;
  if (!logger.print_to_console && !logger.generate_log_file) {
    fprintf(stderr, "Warning: Logger is configured to neither print to console nor generate log file. No logs will be emitted.\n");
  }
  if (generate_log_file) {
    if (log_filename != NULL && strlen(log_filename) >= LOG_FILENAME_MAX_LENGTH) {
      fprintf(stderr, "Warning: Log filename exceeds maximum length. Using default timestamp-based filename instead.\n");
      log_filename = NULL; // Reset to trigger default filename generation (timestamp)
    }
    if (log_filename == NULL) {
      // Set to default log filename with timestamp
      time_t now;
      time(&now);
      struct tm* time_info = localtime(&now);
      strftime(logger.log_filename, sizeof(logger.log_filename), "log_%Y-%m-%d %H%M%S.log", time_info);
    } else {
      snprintf(logger.log_filename, sizeof(logger.log_filename), "%s", log_filename);
    }
  }
  FILE* log_file = fopen(logger.log_filename, "a");
  if (log_file == NULL) {
    fprintf(stderr, "Error: failed to create log file.\n");
    // TODO: Throw an exception here instead of just printing an error message. 
    return logger;
  }
  logger.verbosity = verbosity;
  fclose(log_file);
  return logger;
}



#endif
