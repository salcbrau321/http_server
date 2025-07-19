#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "http_request.h"
#include "http_response.h"

typedef struct HttpServer HttpServer;
typedef void (*HttpHandler)(HttpRequest*, HttpResponse*);

HttpServer *http_server_new(int port);
void http_server_on(HttpServer *srv,
                    const char *method,
                    const char *path_template,
                    HttpHandler handler);
void http_server_run(HttpServer *srv);
void http_server_free(HttpServer *srv);

#endif
