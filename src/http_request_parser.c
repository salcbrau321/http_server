#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "http_request_parser.h"
#include "http_request.h"
#include "header_map.h"
#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>

#define BUF_SIZE 1024

HttpRequestParser* http_request_parser_new() {
    HttpRequestParser* parser = malloc(sizeof(HttpRequestParser));
    if (!parser) return NULL;
    parser->state = METHOD;
    parser->content_length = 0;
    parser->bytes_read = 0;
    return parser;
}

ParseResult http_request_parser_execute(HttpRequestParser* parser, const char* data, size_t len) {
    ParseResult r = {0}; 
    r.status = PARSE_INCOMPLETE;
    r.http_error = HTTP_ERR_NONE;
    r.parser_error = PERR_NONE;
    r.error_message = NULL;
    r.bytes_consumed = 0;
   
    r.request = http_request_new();
    if (!r.request) {
        r.status = PARSE_ERROR_INTERNAL;
        r.parser_error = PERR_OOM;
        r.error_message = "Out of memory allocating HttpRequest";
        return r;
    }
    
    HttpParserState parser_state = parser->state;
    char buffer[BUF_SIZE];
    size_t blen = 0;
    char name_buffer[BUF_SIZE];
    size_t nlen = 0;

    for (size_t i = 0; i < len; i++) {
        char c = data[i];
        switch(parser_state) {
            case METHOD:
                if (c == ' ') {
                    parser_state = METHOD_SPACE;
                } else if (c == "\r") {
                    handle_request_error(r, HTTP_ERR_BAD_REQUEST, "Invalid new line while parsing method");
                } else {
                    buffer_append(buffer, &blen, BUF_SIZE, c);
                }
                break;

            case METHOD_SPACE:
                if (blen == 0) {
                    handle_request_error(r, HTTP_ERR_BAD_REQUEST, "Method is missing");
                }
                r.request->method = buffer_copy(buffer);
                buffer_clear(buffer, &blen);
                buffer_append(buffer, &blen, BUF_SIZE, c);
                parser_state = TARGET;
                break;

            case TARGET:
                if (c == ' ') {
                    parser_state = TARGET_SPACE;
                } else {
                    buffer_append(buffer, &blen, BUF_SIZE, c);
                }
                break;

            case TARGET_SPACE:
                r.request->path = buffer_copy(buffer);
                buffer_clear(buffer, &blen);
                buffer_append(buffer, &blen, BUF_SIZE, c);
                parser_state = HTTP_VERSION;
                break;

            case HTTP_VERSION:
                if (c == '\r') {
                    parser_state = REQLINE_ALMOST_DONE;
                } else {
                    buffer_append(buffer, &blen, BUF_SIZE, c);
                }
                break;

            case REQLINE_ALMOST_DONE:
                r.request->version = buffer_copy(buffer);
                buffer_clear(buffer, &blen);
                buffer_append(buffer, &blen, BUF_SIZE, c);
                if (c == '\n') {
                    parser_state = REQLINE_DONE;
                }
                break;

            case REQLINE_DONE:
                parser_state = HEADER_NAME;
                buffer_append(name_buffer, &nlen, BUF_SIZE, c);
                break;

            case HEADER_NAME:
                if (c == ':') {
                    parser_state = HEADER_COLON;
                } else {
                    buffer_append(name_buffer, &nlen, BUF_SIZE, c);
                }
                break;

            case HEADER_COLON:
                parser_state = HEADER_VALUE;
                buffer_append(buffer, &blen, BUF_SIZE, c);
                break;

            case HEADER_VALUE:
                if (c != ' ' || buffer[0] != '\0') {
                    if (c == '\r') {
                        parser_state = HEADER_ALMOST_DONE;
                    } else {
                        buffer_append(buffer, &blen, BUF_SIZE, c);
                    }
                }
                break;

            case HEADER_ALMOST_DONE:
                if (nlen == 4 && strncasecmp(name_buffer, "Host", 4) == 0) {
                    r.request->host = buffer_copy(buffer);
                }
                else if (nlen == 14 && strncasecmp(name_buffer, "Content-Length", 14) == 0) {
                    r.request->content_length = buffer_copy(buffer);
                }
                else {
                    trim(name_buffer);
                    trim(buffer);
                    header_map_add(r.request->headers, name_buffer, buffer); 
                }
                parser_state = HEADER_DONE;
                break;

            case HEADER_DONE:
                buffer_clear(buffer, &blen);
                buffer_clear(name_buffer, &nlen);
                if (c == '\r') {
                    parser_state = HEADER_SECTION_ALMOST_DONE;
                } else {
                    parser_state = HEADER_NAME;
                    buffer_append(name_buffer, &nlen, BUF_SIZE, c);
                }
                break;

            case HEADER_SECTION_ALMOST_DONE:
                if (parse_long(r.request->content_length) > 0) {
                    parser_state = HEADER_SECTION_DONE;
                } else {
                    parser_state = COMPLETE;
                }
                break;

            case HEADER_SECTION_DONE:
                parser_state = BODY;
                buffer_append(buffer, &blen, BUF_SIZE, c);
                break;

            case BODY:
                buffer_append(buffer, &blen, BUF_SIZE, c);
                if (blen == parse_long(r.request->content_length)) {
                    parser_state = COMPLETE;
                    r.request->body = buffer_copy(buffer);
                }
                break;
        }
        // means error was encountered, so we must exit
        if (r.status != PARSE_INCOMPLETE) {
            break;
        }
    }
    for (size_t i = 0; i < r.request->headers->count; i++) {
        for (Header *e = r.request->headers->items[i]; e; e = e->next) {
            printf("%s: %s\n", e->name, e->value);
        }
    }
    if (parser_state == COMPLETE) {
        char *q = strchr(r.request->path, '?');
        if (q) {
            *q = '\0';
            r.request->path = strdup(r.request->path);
            char *qs = q + 1;
            int qc = 0;
            char *param = strtok(qs, "&");
            while (param && qc < MAX_QUERY_PARAMS) {
                char *eq = strchr(param, '=');
                if (eq) {
                    *eq = '\0';
                    strncpy(r.request->query_name[qc], param, MAX_PARAM_NAME-1);
                    strncpy(r.request->query_val[qc], eq+1, MAX_PARAM_VALUE-1);
                    qc++;
                }
                param = strtok(NULL, "&");
            }
            r.request-> query_count = qc;
        } else {
            r.request->path = strdup(r.request->path);
            r.request->query_count = 0;
        }
    }
    parser->state = parser_state;
    r.status = PARSE_OK;
    return r;
}

void handle_request_error(ParseResult r, HttpErrorCode error,  char* message) {
    r.status = PARSE_ERROR_REQUEST;
    r.http_error = error;
    r.error_message = message;
}

void handle_parser_error(ParseResult r, ParserErrorCode error,  char* message) {
    r.status = PARSE_ERROR_INTERNAL;
    r.http_error = error;
    r.error_message = message;
}
