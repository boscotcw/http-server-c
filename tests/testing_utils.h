#include <stdio.h>

void print_test_passed() { printf("\n---------- TEST PASSED ----------\n"); }

void remove_test_file(const char *filename) {
  if (remove(filename)) {
    printf("Encountered error when removing %s test file. Please remove it manually.\n");
  }
}