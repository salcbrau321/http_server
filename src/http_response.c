#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "http_response.h"

struct HttpResponse {
    int client_fd;
};

HttpResponse *http_response_new(int client_fd) {
    HttpResponse *res = malloc(sizeof(*res));
    if (!res) return NULL;
    res->client_fd = client_fd;
    return res;
}

void http_response_send_json(HttpResponse *res, int status_code, const char *json_body) {
    char header[512];
    size_t body_len = strlen(json_body);
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, body_len);
    send(res->client_fd, header, header_len, 0);
    send(res->client_fd, json_body, body_len, 0);
}

void http_response_free(HttpResponse *res) {
    if (!res) return;
    close(res->client_fd);
    free(res);
}
