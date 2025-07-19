#ifndef ROUTER_H
#define ROUTER_H

#include "http_server.h"

typedef struct{
    char  *method;
    char  *path_template;
    HttpHandler handler;
} Route;

void router_add(const char *method, const char *path_template, HttpHandler handler); 
HttpHandler router_match(HttpRequest *req);

#endif
