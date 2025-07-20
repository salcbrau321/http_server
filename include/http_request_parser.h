#ifndef HTTP_REQUEST_PARSER_H
#define HTTP_REQUEST_PARSER_H

#include "http_request.h"

typedef enum {
    METHOD,
    METHOD_SPACE,
    TARGET,
    TARGET_SPACE,
    HTTP_VERSION,
    REQLINE_ALMOST_DONE,
    REQLINE_DONE,
    HEADER_NAME,
    HEADER_COLON,
    HEADER_VALUE,
    HEADER_ALMOST_DONE,
    HEADER_DONE,
    HEADER_SECTION_ALMOST_DONE,
    HEADER_SECTION_DONE,
    BODY,
    COMPLETE,
    ERROR
} HttpParserState;

typedef struct {
    HttpParserState state;
    size_t content_length;
    size_t bytes_read;
} HttpRequestParser;

HttpRequestParser* http_request_parser_new();
int http_request_parser_execute(HttpRequestParser* parser, const char* data, size_t len, HttpRequest* req);

#endif
