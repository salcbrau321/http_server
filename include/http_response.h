#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

typedef struct HttpResponse HttpResponse;

HttpResponse *http_response_new(int client_fd);
void http_response_send_json(HttpResponse *res, int status_code, const char *json_body);
void http_response_free(HttpResponse *res);

#endif
