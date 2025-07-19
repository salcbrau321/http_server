#include <string.h>
#include "router.h"
#include "http_request.h"

#define MAX_ROUTES 100
#define MAX_SEGMENTS 16

static Route routes[MAX_ROUTES];
static int   route_count = 0;

void router_add(const char *method, const char *path_template, HttpHandler handler) {
    if (route_count < MAX_ROUTES) {
        routes[route_count].method = strdup(method);
        routes[route_count].path_template = strdup(path_template);
        routes[route_count].handler = handler;
        route_count++;
    }
}

HttpHandler router_match(HttpRequest *req) {
    char req_copy[256];
    strncpy(req_copy, req->path, sizeof(req_copy));
    char *req_segs[MAX_SEGMENTS];
    int   req_n = 0;
    char *tok = strtok(req_copy, "/");
    while (tok && req_n < MAX_SEGMENTS) {
        req_segs[req_n++] = tok;
        tok = strtok(NULL, "/");
    }

    for (int i = 0; i < route_count; i++) {
        char tpl_copy[256];
        strncpy(tpl_copy, routes[i].path_template, sizeof(tpl_copy));
        char *tpl_segs[MAX_SEGMENTS];
        int   tpl_n = 0;
        char *t = strtok(tpl_copy, "/");
        while (t && tpl_n < MAX_SEGMENTS) {
            tpl_segs[tpl_n++] = t;
            t = strtok(NULL, "/");
        }

        if (tpl_n != req_n) continue;

        req->query_count = 0;
        int ok = 1;
        for (int j = 0; j < tpl_n; j++) {
            if (tpl_segs[j][0] == ':') {
                const char *name = tpl_segs[j] + 1;
                const char *val  = req_segs[j];
                strncpy(req->query_name[req->query_count], name, MAX_PARAM_NAME-1);
                strncpy(req->query_val[req->query_count], val, MAX_PARAM_VALUE-1);
                req->query_count++;
            } else if (strcmp(tpl_segs[j], req_segs[j]) != 0) {
                ok = 0;
                break;
            }
        }

        if (ok && strcmp(req->method, routes[i].method) == 0) {
            return routes[i].handler;
        }
    }
    return NULL;
}

