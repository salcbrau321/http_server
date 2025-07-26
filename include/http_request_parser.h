#ifndef HTTP_REQUEST_PARSER_H
#define HTTP_REQUEST_PARSER_H

#include "http_request.h"

typedef enum {
    PARSE_OK,
    PARSE_INCOMPLETE,
    PARSE_ERROR_INTERNAL,
    PARSE_ERROR_REQUEST
} ParseStatus;

typedef enum {
    HTTP_ERR_NONE = 0,
    HTTP_ERR_BAD_REQUEST = 400,
    HTTP_ERR_URI_TOO_LONG = 414,
    HTTP_ERR_VERSION_NOT_SUP = 505,
    HTTP_ERR_HEADER_TOO_LARGE = 431,
    HTTP_ERR_LENGTH_REQUIRED = 411
} HttpErrorCode;

typedef enum {
    PERR_NONE,
    PERR_OOM,
    PERR_STATE_CORRUPT,
    PERR_HEADER_OVERFLOW
} ParserErrorCode;

typedef struct {
    ParseStatus status;
    
    HttpErrorCode http_error;
    ParserErrorCode parser_error;
    const char* error_message;

    HttpRequest* request;
    size_t bytes_consumed;
} ParseResult;

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
ParseResult http_request_parser_execute(HttpRequestParser* parser, const char* data, size_t len);

#endif
