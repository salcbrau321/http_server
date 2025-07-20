#ifndef UTILS_H
#define UTLS_H

#include <stddef.h>
#include <stdbool.h>

void trim(char *s);
long safe_parse_long(const char* s);
void buffer_clear(char* buffer, size_t* len);
bool buffer_append(char* buffer, size_t* len, size_t max_len, char c);
char* buffer_copy(const char* buffer);

#endif
