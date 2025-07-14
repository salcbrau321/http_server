#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "request.h"

typedef void (*RouteHandler)(Request *, Response *);

void http_register_handler(const char *method, const char *path, RouteHandler handler);
void http_server_listen(int port);

#endif