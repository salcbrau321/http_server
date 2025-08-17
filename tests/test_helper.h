#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <string.h>
#include "http_request_parser.h"

static inline ParseResult parse_request(const char *raw) {
    HttpRequestParser *p = http_request_parser_new();
    ParseResult res = http_request_parser_execute(p, raw, strlen(raw));
    free(p);
    return res;
}

#endif 

