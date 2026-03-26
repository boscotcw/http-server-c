#ifndef NETWORK_HELPERS_H
#define NETWORK_HELPERS_H

#include <fcntl.h>
#include <stdio.h>

int set_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl(F_GETFL)");
    return -1;
  }

  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl(F_SETFL)");
    return -1;
  }

  return 0;
}

#endif
