#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "http_request_parser.h"
#include "http_request.h"
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>

#define BUF_SIZE 1024

static inline void trim(char *s) {
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);

    size_t len = strlen(s);
    while (len && isspace((unsigned char)s[len-1])) {
        s[--len] = '\0';
    }
}


static inline void buffer_clear(char* buffer, size_t* len) {
    *len = 0;
    buffer[0] = '\0';
}

static inline long parse_long(const char *s) {
    if (!s) return 0;

    char *end;
    errno = 0;                   
    long val = strtol(s, &end, 10);
    if (end == s) {
        return 0;
    }
    if (errno == ERANGE || val < INT_MIN || val > INT_MAX) {
        return 0;
    }
    return val;
}

static inline char* copy_token(const char *buffer) {
    size_t len = strlen(buffer);
    char* dst = malloc(len + 1);
    if (!dst) {
        return NULL;
    }
    memcpy(dst, buffer, len + 1);
    trim(dst);
    return dst;
}

static inline bool buffer_append(char* buffer, size_t* len, size_t max_len, char c) {
    if (*len + 1 >= max_len) {
        return false;
    }
    buffer[(*len)++] = c;
    buffer[*len] = '\0';
    return true;
}

HttpRequestParser* http_request_parser_new() {
    HttpRequestParser* parser = malloc(sizeof(HttpRequestParser));
    if (!parser) return NULL;
    parser->state = METHOD;
    parser->content_length = 0;
    parser->bytes_read = 0;
    return parser;
}

int http_request_parser_execute(HttpRequestParser* parser, const char* data, size_t len, HttpRequest* req) {
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
                } else {
                    buffer_append(buffer, &blen, BUF_SIZE, c);
                }
                break;

            case METHOD_SPACE:
                req->method = copy_token(buffer);
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
                req->path = copy_token(buffer);
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
                req->version = copy_token(buffer);
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
                    req->host = copy_token(buffer);
                }
                else if (nlen == 14 && strncasecmp(name_buffer, "Content-Length", 14) == 0) {
                    req->content_length = copy_token(buffer);
                }
                else if (nlen == 12 && strncasecmp(name_buffer, "Content-Type", 12) == 0) {
                    req->content_type = copy_token(buffer);
                }
                else if (nlen == 17 && strncasecmp(name_buffer, "Transfer-Encoding", 17) == 0) {
                    req->transfer_encoding = copy_token(buffer);
                }
                else if (nlen == 10 && strncasecmp(name_buffer, "Connection", 10) == 0) {
                    req->connection = copy_token(buffer);
                }
                else {
                
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
                if (parse_long(req->content_length) > 0) {
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
                if (blen == parse_long(req->content_length)) {
                    parser_state = COMPLETE;
                    req->body = copy_token(buffer);
                }
                break;
        }
    }

    if (parser_state == COMPLETE) {
        char *q = strchr(req->path, '?');
        if (q) {
            *q = '\0';
            req->path = strdup(req->path);
            char *qs = q + 1;
            int qc = 0;
            char *param = strtok(qs, "&");
            while (param && qc < MAX_QUERY_PARAMS) {
                char *eq = strchr(param, '=');
                if (eq) {
                    *eq = '\0';
                    strncpy(req->query_name[qc], param, MAX_PARAM_NAME-1);
                    strncpy(req->query_val[qc], eq+1, MAX_PARAM_VALUE-1);
                    qc++;
                }
                param = strtok(NULL, "&");
            }
            req-> query_count = qc;
        } else {
            req->path = strdup(req->path);
            req->query_count = 0;
        }
    }
    parser->state = parser_state;
    return 0;
}

