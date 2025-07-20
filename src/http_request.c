#include <string.h>
#include "http_request.h"

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
    if (req->content_type) free(req->content_type);
    if (req->transfer_encoding) free(req->transfer_encoding);
    if (req->connection) free(req->connection);
    if (req->body) free(req->body);
    free(req);
}

