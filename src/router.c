#include <string.h>
#include "router.h"

#define MAX_ROUTES 100

typedef struct {
    const char *method;
    const char *path;
    RouteHandler handler;
} Route;

static Route routes[MAX_ROUTES];
static int route_count = 0;

void router_add(const char *method, const char *path, RouteHandler handler) {
    if (route_count < MAX_ROUTES) {
        routes[route_count++] = (Route){method, path, handler};
    }
}

RouteHandler router_match(const char *method, const char *path) {
    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].method, method) == 0 && strcmp(routes[i].path, path) == 0) {
            return routes[i].handler;
        }
    }
    return NULL;
}