#ifndef ROUTER_H
#define ROUTER_H

#include "request.h"

typedef void (*RouteHandler)(Request *, Response *);

void router_add(const char *method, const char *path, RouteHandler handler);
RouteHandler router_match(const char *method, const char *path);

#endif