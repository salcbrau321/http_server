#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#define MAX_QUERY_PARAMS 16
#define MAX_PARAM_NAME 32
#define MAX_PARAM_VALUE 128

typedef struct {
    char *method;
    char *path;
    char *body;

    int query_count;
    char query_name[MAX_QUERY_PARAMS][MAX_PARAM_NAME];
    char query_val[MAX_QUERY_PARAMS][MAX_PARAM_VALUE];
} HttpRequest;

const char *http_request_query(const HttpRequest *req, const char *name);

#endif
