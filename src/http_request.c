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
