#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "network.h"
#include "http_parser.h"
#include "http_builder.h"
#include "data.h"
#include <stddef.h>
#include <stdbool.h>

struct HttpServer {
    struct NetworkIO network_io_module;
    struct HttpParser http_parser_module;
    struct HttpBuilder http_builder_module;
    struct DataIO data_io_module;
};

/*
 * @brief Since the HttpServer is hardcoded to use localhost,
 * only port is required to be passed in.
 *
 * @param port A free port in the range: [ x - x ]
 */
struct HttpServer* get_http_server(int port) {
    // malloc an http server.
    // construct and init modules
    // starts the http server 
    // starting means to start listening for connections on the listen socket,
    // and accepting connections from clients.
    return NULL;
}

void free_http_server(struct HttpServer* http_server) {
   
}

#endif
