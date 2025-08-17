#ifndef HTTP_REQUEST_PARSER_H
#define HTTP_REQUEST_PARSER_H

#include "http_request.h"

#define BUF_SIZE 1024

typedef enum {
    HTTP_STATE_IDLE,
    HTTP_STATE_METHOD,
    HTTP_STATE_TARGET,
    HTTP_STATE_HTTP_VERSION,
    HTTP_STATE_REQLINE_ALMOST_DONE,
    HTTP_STATE_REQLINE_DONE,
    HTTP_STATE_HEADER_NAME,
    HTTP_STATE_HEADER_COLON,
    HTTP_STATE_HEADER_VALUE,
    HTTP_STATE_HEADER_ALMOST_DONE,
    HTTP_STATE_HEADER_DONE,
    HTTP_STATE_HEADER_SECTION_ALMOST_DONE,
    HTTP_STATE_HEADER_SECTION_DONE,
    HTTP_STATE_BODY,
    HTTP_STATE_COMPLETE,
    HTTP_STATE_ERROR
} HttpParserState;

typedef enum {
    PARSE_COMPLETE,
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
    HttpParserState parser_state;

    HttpErrorCode http_error;
    ParserErrorCode parser_error;
    const char* error_message;

    HttpRequest* request;
    size_t bytes_consumed;
} ParseResult;

typedef struct {
    HttpParserState state;
    HttpRequest* request;
    size_t content_length;
    size_t bytes_read;
    char buffer[BUF_SIZE];
    size_t blen;
    char name_buffer[BUF_SIZE];
    size_t nlen;
} HttpRequestParser;

HttpRequestParser* http_request_parser_new();
ParseResult http_request_parser_execute(HttpRequestParser* parser, const char* data, size_t len);

#endif
