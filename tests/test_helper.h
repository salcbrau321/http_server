#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <string.h>
#include "http_request_parser.h"

static inline ParseResult parse_request(const char *raw) {
    HttpRequestParser *parser = http_request_parser_new();
    ParseResult res = http_request_parser_execute(parser, raw, strlen(raw));
    free(parser);
    return res;
}

#endif 

