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


#define MAX_METHOD_LEN 16

static bool validate_method_step(ParseResult* r, char c, size_t len);

HttpRequestParser* http_request_parser_new() {
    HttpRequestParser* parser = malloc(sizeof(HttpRequestParser));
    if (!parser) return NULL;
    parser->state = HTTP_STATE_IDLE;
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
 
    if (parser->state == HTTP_STATE_IDLE) {
        parser->request = http_request_new();
        parser->state = HTTP_STATE_METHOD;
        memset(parser->buffer, 0, BUF_SIZE);
        parser->blen = 0;
        memset(parser->name_buffer, 0, BUF_SIZE);
        parser->name_buffer[BUF_SIZE];
        parser->nlen = 0;
        if (!parser->request) {
            r.status = PARSE_ERROR_INTERNAL;
            r.parser_error = PERR_OOM;
            r.error_message = "Out of memory allocating HttpRequest";
            return r;
        }
    }
    
    HttpParserState parser_state = parser->state;

    for (size_t i = 0; i < len; i++) {
        char c = data[i];
        r.bytes_consumed++;
        switch(parser_state) {
            case HTTP_STATE_METHOD:
                if (!validate_method_step(&r, c, parser->blen)) {
                    break;
                }
                if (c == ' ') {
                    parser_state = HTTP_STATE_TARGET;
                    parser->request->method = buffer_copy(parser->buffer);
                    buffer_clear(parser->buffer, &parser->blen);
                } else {
                    buffer_append(parser->buffer, &parser->blen, BUF_SIZE, c);
                }
                break;
            case HTTP_STATE_TARGET:
                if (c == ' ') {
                    parser_state = HTTP_STATE_HTTP_VERSION;
                    parser->request->path = buffer_copy(parser->buffer);
                    buffer_clear(parser->buffer, &parser->blen);
                } else {
                    buffer_append(parser->buffer, &parser->blen, BUF_SIZE, c);
                }
                break;
            case HTTP_STATE_HTTP_VERSION:
                if (c == '\r') {
                    parser_state = HTTP_STATE_REQLINE_ALMOST_DONE;
                } else {
                    buffer_append(parser->buffer, &parser->blen, BUF_SIZE, c);
                }
                break;

            case HTTP_STATE_REQLINE_ALMOST_DONE:
                parser->request->version = buffer_copy(parser->buffer);
                buffer_clear(parser->buffer, &parser->blen);
                buffer_append(parser->buffer, &parser->blen, BUF_SIZE, c);
                if (c == '\n') {
                    parser_state = HTTP_STATE_REQLINE_DONE;
                }
                break;

            case HTTP_STATE_REQLINE_DONE:
                parser_state = HTTP_STATE_HEADER_NAME;
                buffer_append(parser->name_buffer, &parser->nlen, BUF_SIZE, c);
                break;

            case HTTP_STATE_HEADER_NAME:
                if (c == ':') {
                    parser_state = HTTP_STATE_HEADER_COLON;
                } else {
                    buffer_append(parser->name_buffer, &parser->nlen, BUF_SIZE, c);
                }
                break;

            case HTTP_STATE_HEADER_COLON:
                parser_state = HTTP_STATE_HEADER_VALUE;
                buffer_append(parser->buffer, &parser->blen, BUF_SIZE, c);
                break;

            case HTTP_STATE_HEADER_VALUE:
                if (c != ' ' || parser->buffer[0] != '\0') {
                    if (c == '\r') {
                        parser_state = HTTP_STATE_HEADER_ALMOST_DONE;
                    } else {
                        buffer_append(parser->buffer, &parser->blen, BUF_SIZE, c);
                    }
                }
                break;

            case HTTP_STATE_HEADER_ALMOST_DONE:
                if (parser->nlen == 4 && strncasecmp(parser->name_buffer, "Host", 4) == 0) {
                    parser->request->host = buffer_copy(parser->buffer);
                }
                else if (parser->nlen == 14 && strncasecmp(parser->name_buffer, "Content-Length", 14) == 0) {
                    parser->request->content_length = buffer_copy(parser->buffer);
                }
                else {
                    trim(parser->name_buffer);
                    trim(parser->buffer);
                    header_map_add(parser->request->headers, parser->name_buffer, parser->buffer); 
                }
                parser_state = HTTP_STATE_HEADER_DONE;
                break;

            case HTTP_STATE_HEADER_DONE:
                buffer_clear(parser->buffer, &parser->blen);
                buffer_clear(parser->name_buffer, &parser->nlen);
                if (c == '\r') {
                    parser_state = HTTP_STATE_HEADER_SECTION_ALMOST_DONE;
                } else {
                    parser_state = HTTP_STATE_HEADER_NAME;
                    buffer_append(parser->name_buffer, &parser->nlen, BUF_SIZE, c);
                }
                break;

            case HTTP_STATE_HEADER_SECTION_ALMOST_DONE:
                if (parse_long(parser->request->content_length) > 0) {
                    parser_state = HTTP_STATE_HEADER_SECTION_DONE;
                } else {
                    parser_state = HTTP_STATE_COMPLETE;
                }
                break;

            case HTTP_STATE_HEADER_SECTION_DONE:
                parser_state = HTTP_STATE_BODY;
                buffer_append(parser->buffer, &parser->blen, BUF_SIZE, c);
                break;

            case HTTP_STATE_BODY:
                buffer_append(parser->buffer, &parser->blen, BUF_SIZE, c);
                if (parser->blen == parse_long(parser->request->content_length)) {
                    parser_state = HTTP_STATE_COMPLETE;
                    parser->request->body = buffer_copy(parser->buffer);
                }
                break;
        }
        if (r.status == PARSE_ERROR_INTERNAL || r.status == PARSE_ERROR_REQUEST) {
            break;
        }
        if (parser_state == HTTP_STATE_COMPLETE) {
            break;
        }
    }
    
    if (parser_state == HTTP_STATE_COMPLETE) {
        char *q = strchr(parser->request->path, '?');
        if (q) {
            *q = '\0';
            char* old = parser->request->path;
            parser->request->path = strdup(old);
            free(old);
            char *qs = q + 1;
            int qc = 0;
            char *param = strtok(qs, "&");
            while (param && qc < MAX_QUERY_PARAMS) {
                char *eq = strchr(param, '=');
                if (eq) {
                    *eq = '\0';
                    strncpy(parser->request->query_name[qc], param, MAX_PARAM_NAME-1);
                    strncpy(parser->request->query_val[qc], eq+1, MAX_PARAM_VALUE-1);
                    qc++;
                }
                param = strtok(NULL, "&");
            }
            parser->request-> query_count = qc;
        } else {
            parser->request->path = strdup(parser->request->path);
            parser->request->query_count = 0;
        }
        r.request = parser->request;
        parser->request = NULL;
        r.status = PARSE_OK;
        r.parser_state = parser_state;
        parser_state = HTTP_STATE_IDLE;
    } else if (parser_state != HTTP_STATE_ERROR) {
        r.status = PARSE_INCOMPLETE;
        r.parser_state = parser_state;
    }
    parser->state = parser_state;
    return r;
}

void handle_request_error(ParseResult* r, HttpErrorCode error,  char* message) {
    r->status = PARSE_ERROR_REQUEST;
    r->http_error = error;
    r->error_message = message;
}

void handle_parser_error(ParseResult* r, ParserErrorCode error,  char* message) {
    r->status = PARSE_ERROR_INTERNAL;
    r->parser_error = error;
    r->error_message = message;
}

bool validate_method_step(ParseResult* r, char c, size_t len)
{
    if (c == ' ' && len == 0) {
        handle_request_error(r, HTTP_ERR_BAD_REQUEST, "Method is missing");
        return false;
    } 

    if (c != ' ' && !is_tchar(c)) {
        handle_request_error(
            r, 
            HTTP_ERR_BAD_REQUEST,
            "Invalid character found in header");
        return false;
    }

    if (len + (c != ' ' ? 1 : 0) > MAX_METHOD_LEN) {
        handle_request_error(r, HTTP_ERR_BAD_REQUEST, "Method too long");
        return false;
    }
    return true;
}

