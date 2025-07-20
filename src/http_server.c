#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "http_server.h"
#include "router.h"
#include "http_request.h"
#include "http_response.h"
#include "http_request_parser.h"

struct HttpServer {
    int port;
    int listen_fd;
};

HttpServer *http_server_new(int port) {
    HttpServer *srv = malloc(sizeof(*srv));
    if (!srv) return NULL;
    srv->port = port;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { 
        free(srv);
        return NULL;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        free(srv);
        return NULL;
    }

    if (listen(fd, 10) < 0) {
        close(fd);
        free(srv);
        return NULL;
    }

    srv->listen_fd = fd;
    printf("Listening on port %d...\n", port);
    return srv;
}

void http_server_on(HttpServer *srv, const char *method, const char *path_template, HttpHandler handler) {
    (void)srv;
    router_add(method, path_template, handler);
}

void http_server_run(HttpServer *srv) {
    while (1) {
        int client_fd = accept(srv->listen_fd, NULL, NULL);
        if (client_fd < 0) continue;

        char buffer[4096] = {0};
        ssize_t n = read(client_fd, buffer, sizeof(buffer)-1);
        if (n <= 0) {
            close(client_fd);
            continue;
        }

        HttpRequest* req = http_request_new();
        HttpRequestParser* parser = http_request_parser_new();
        http_request_parser_execute(parser, buffer, (size_t)n, req); 
        HttpHandler h = router_match(req);
        HttpResponse *res = http_response_new(client_fd);
        if (h) {
            h(req, res);
        } else {
            http_response_send_json(res, 404, "{\"error\":\"Not Found\"}");
        }
        http_response_free(res);
        http_request_free(req);
    }
}

void http_server_free(HttpServer *srv) {
    if (!srv) return;
    close(srv->listen_fd);
    free(srv);
}
