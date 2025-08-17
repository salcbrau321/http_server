#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "utils.h"
#include <stdbool.h>
#include <limits.h>

void trim(char *s) {
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);

    size_t len = strlen(s);
    while (len && isspace((unsigned char)s[len-1])) {
        s[--len] = '\0';
    }
}

long parse_long(const char *s) {
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

void buffer_clear(char* buffer, size_t* len) {
    *len = 0;
    buffer[0] = '\0';
}

bool buffer_append(char* buffer, size_t* len, size_t max_len, char c) {
    if (*len + 1 >= max_len) {
        return false;
    }
    buffer[(*len)++] = c;
    buffer[*len] = '\0';
    return true;
}

char* buffer_copy(const char *buffer) {
    size_t len = strlen(buffer);
    char* dst = malloc(len + 1);
    if (!dst) {
        return NULL;
    }
    memcpy(dst, buffer, len + 1);
    trim(dst);
    return dst;
}

bool is_tchar(char c) {
    if ((c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9')) return true;

    switch (c) {
        case '!': case '#': case '$': case '%': case '&': case '\'':
        case '*': case '+': case '-': case '.': case '^': case '_':
        case '`': case '|': case '~':
            return true;
        default:
            return false;
     }
}
