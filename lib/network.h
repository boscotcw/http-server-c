#ifndef NETWORK_IO_MODULE_H
#define NETWORK_IO_MODULE_H

#include "http_parser.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define G_MAX_CLIENT_SOCKETS                                                   \
  1000 // This should always be greater than 5, since (usually) the first
       // available fd is 5 for clients. Also, note that going larger may
       // overload the available stack space in your system, as we currently use
       // stack memory.
#define G_NUM_LISTENING_SOCKETS 1 // Always 1 for TCP sockets
#define G_MAX_SOCKETS (G_MAX_CLIENT_SOCKETS + G_NUM_LISTENING_SOCKETS)

#define G_MAX_METHOD_LEN 6 // GET, POST, PUT, DELETE
#define G_SP_LEN 1

#define G_BUFFER_SCALING_FACTOR 2
#define G_MAX_BUFFER_SIZE                                                      \
  ((G_MAX_METHOD_LEN + G_SP_LEN + G_MAX_URI_LEN) * G_BUFFER_SCALING_FACTOR)

struct Connection {
  char buffer[G_MAX_BUFFER_SIZE];
  int check_offset; // This offset (suggested by Gemini) is used to avoid wasted
                    // linear scans to find the first available CLRF.
  int write_offset;
};

struct NetworkIO {
  bool initialised;
  int port;
  int num_sockets; // Equivalent to saying num_fds, i.e. number of clients plus
                   // one listening socket.
                   // num_sockets should be at most G_MAX_SOCKETS.
  struct Connection connections[G_MAX_CLIENT_SOCKETS];
};

/*
 * @brief Initialises a NetworkIO module.
 */
void init_network_io(struct NetworkIO *network_io_module, int port) {
  network_io_module->initialised = true;
  network_io_module->num_sockets = G_NUM_LISTENING_SOCKETS;

  assert(port >= 49152 && port <= 65535 &&
         "please use a port in the range [49152-65535].");
  network_io_module->port = port;
}

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

void run(struct NetworkIO *network_io_module) {
  assert(network_io_module->initialised);
  // This is a TCP socket: https://man7.org/linux/man-pages/man7/ip.7.html
  int listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (listen_socket == -1) {
    err(EXIT_FAILURE, "getting a listen_socket failed");
  }
  printf("Created listen socket.\n");

  if (set_non_blocking(listen_socket) == -1) {
    err(EXIT_FAILURE, "setting listen_socket to non-blocking failed");
  }
  printf("Set listen socket to non blocking.\n");

  int opt = 1;
  if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt)) == -1) {
    perror("setsockopt failed for listen_socket");
    exit(EXIT_FAILURE);
  }
  printf("Set socket options to re-use address and re-use port.\n");

  // Using designated initializers will automatically zero out paddings, so we
  // don't need to memset to 0.
  assert(network_io_module->port >= 49152 && network_io_module->port <= 65535 &&
         "please use a port in the range [49152-65535].");
  // TODO: Expand to public application and replace loopback address
  // restriction.
  struct sockaddr_in server_addr = {.sin_family = AF_INET,
                                    .sin_port = htons(network_io_module->port),
                                    .sin_addr = htonl(INADDR_LOOPBACK)};

  printf("Note: currently hardcoded to using loopback address.\n");

  if (bind(listen_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) == -1) {
    err(EXIT_FAILURE, "binding the listen_socket failed");
  }
  printf("Binding listen socket to loopback address.\n");

  if (listen(listen_socket, G_MAX_CLIENT_SOCKETS) == -1) {
    err(EXIT_FAILURE, "listen for listen_socket failed");
  }
  printf("Listen mode set for listen socket.\n");

  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd == -1) {
    err(EXIT_FAILURE, "epoll_create1 failed");
  }
  printf("epoll instance created.\n");

  // Reusable structure to help register the listen_socket and future
  // client_sockets.
  struct epoll_event epoll_event;
  struct epoll_event epoll_events[G_MAX_SOCKETS];

  // Register the listen_socket.
  epoll_event.events = EPOLLIN;
  epoll_event.data.fd = listen_socket;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket, &epoll_event) == -1) {
    err(EXIT_FAILURE, "epoll_ctl failed for listen_socket");
  }
  printf("Registed listen socket to epoll instance.\n");

  int num_ready_fds = 0;
  while (true) {
    num_ready_fds = epoll_wait(epoll_fd, epoll_events, G_MAX_SOCKETS, -1);
    printf("Event received. Processing batch of ready fds...\n");

    if (num_ready_fds == -1) {
      err(EXIT_FAILURE, "epoll_wait failed");
    }

    for (int i = 0; i < num_ready_fds; ++i) {
      if (epoll_events[i].data.fd == listen_socket) {
        struct sockaddr_in client_addr;

        socklen_t client_addr_len;

        int client_connection_socket = accept(
            listen_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_connection_socket == -1) {
          err(EXIT_FAILURE, "failed to accept a client_connection_socket");
        }

        if (network_io_module->num_sockets == G_MAX_SOCKETS) {
          perror("rejecting new client as G_MAX_SOCKETS reached");
          close(client_connection_socket);
          continue;
        }

        network_io_module->num_sockets += 1;
        printf(
            "Accepted new client connection: fd %d. Total client count: %d\n",
            client_connection_socket, network_io_module->num_sockets - 1);

        if (set_non_blocking(client_connection_socket) == -1) {
          err(EXIT_FAILURE,
              "failed to set a client_connection_socket as non-blocking");
        }
        printf("Set new client connection as non-blocking.\n");

        // A client connection socket will be epolled via an edge-triggered
        // interface.
        epoll_event.events = EPOLLIN | EPOLLET;
        epoll_event.data.fd = client_connection_socket;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_connection_socket,
                      &epoll_event) == -1) {
          err(EXIT_FAILURE, "epoll_ctl failed for client_connection_socket");
        }
        printf("Registered new client to epoll instance.\n");

        struct Connection client_connection = {
            .buffer = {}, .check_offset = 0, .write_offset = 0};

        network_io_module->connections[client_connection_socket] =
            client_connection;
        printf("Added new client Connection metadata to NetworkIO module.\n");

      } else {
        int client_connection_socket = epoll_events[i].data.fd;
        char
            recv_buffer[G_MAX_BUFFER_SIZE]; // Does not assume null-termination.

        while (true) {
          int num_bytes =
              recv(client_connection_socket, recv_buffer, G_MAX_BUFFER_SIZE, 0);

          // TODO: Clean up the ugly offset logic!
          if (num_bytes > 0) {
            printf("Number of bytes received from fd %d: %d\n",
                   client_connection_socket, num_bytes);
            if (num_bytes > G_MAX_BUFFER_SIZE ||
                network_io_module->connections[client_connection_socket]
                            .write_offset +
                        num_bytes >=
                    G_MAX_BUFFER_SIZE) {
              // TODO: reject_client as buffer overflow
              printf("Rejected client.");
              break;
            }

            // Append to per-client application buffer
            // TODO: Refactor as a "append" function
            for (int i = 0; i < num_bytes; ++i) {
              network_io_module->connections[client_connection_socket]
                  .buffer[network_io_module
                              ->connections[client_connection_socket]
                              .write_offset +
                          i] = recv_buffer[i];
            }
            printf("Appended recv_buffer to transfer_buffer.\n");

            // TODO: refer to above
            network_io_module->connections[client_connection_socket]
                .write_offset += num_bytes;
            printf("Updated write offset.\n");

          } else if (num_bytes == -1) {
            perror(
                "some error may have occurred during recv from client socket");
            break;

          } else if (num_bytes == 0) {
            network_io_module->num_sockets -= 1;

            assert(network_io_module->num_sockets >= 1);

            struct Connection empty_connection = {
                .buffer = {}, .write_offset = 0, .check_offset = 0};

            network_io_module->connections[client_connection_socket] =
                empty_connection;

            printf("Orderly shutdown of connection by a client during socket "
                   "draining.\n");
            break;
          }

          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("Socket drained for client.\n");
            break;
          }
        }
      }
    }
  }
}

#endif
