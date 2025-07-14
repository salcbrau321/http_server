#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#include "request.h"
#include "router.h"

void res_send_json(Response *res, int status, const char *json) {
    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 %d OK\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %lu\r\n\r\n",
             status, strlen(json));
    send(res->client_fd, header, strlen(header), 0);
    send(res->client_fd, json, strlen(json), 0);
}

void http_register_handler(const char *method, const char *path, RouteHandler handler) {
    router_add(method, path, handler);
}


void http_server_listen(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(sockfd, 10);

    printf("Listening on port %d...\n", port);

    while (1) {
        int client_fd = accept(sockfd, NULL, NULL);
        char buffer[4096] = {0};
        read(client_fd, buffer, sizeof(buffer) - 1);

        char method[8], path[256];
        sscanf(buffer, "%s %s", method, path);

        char *body = strstr(buffer, "\r\n\r\n");
        if (body) body += 4; else body = NULL;

        Request req = {method, path, body};
        Response res = {client_fd};

        RouteHandler handler = router_match(method, path);
        if (handler) {
            handler(&req, &res);
        } else {
            res_send_json(&res, 404, "{\"error\":\"Not Found\"}");
        }

        close(client_fd);
    }
}
