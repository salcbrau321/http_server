#ifndef REQUEST_H
#define REQUEST_H

typedef struct {
    const char *method;
    const char *path;
    const char *body;
} Request;

typedef struct {
    int client_fd;
} Response;

void res_send_json(Response *res, int status_code, const char *json);

#endif