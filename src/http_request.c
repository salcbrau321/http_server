#include <string.h>
#include "http_request.h"
#include "header_map.h"

HttpRequest* http_request_new() {
    HttpRequest* req = calloc(1, sizeof *req);
    if (!req) return NULL;
    req->headers = header_map_new();
    return req;
}

const char *http_request_query(const HttpRequest *req, const char *name) {
    for (int i = 0; i < req->query_count; i++) {
        if (strcmp(req->query_name[i], name) == 0) {
            return req->query_val[i];
        }
    }
    return NULL;
}

void http_request_free(const HttpRequest* req) {
    if (!req) return;
    if (req->method) free(req->method);
    if (req->path) free(req->path);
    if (req->version) free(req->version);
    if (req->host) free(req->host);
    if (req->content_length) free(req->content_length);
    if (req->body) free(req->body);

    header_map_free(req->headers);
    free(req);
}

