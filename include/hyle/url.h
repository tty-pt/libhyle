#ifndef HYLE_URL_H
#define HYLE_URL_H

#include <stddef.h>

/* Split a query-string without modifying input: find `key`, URL-decode the
 * value (%XX hex escapes and '+' as space) into out. Returns bytes written;
 * 0 when the key is absent or an argument is invalid. out is always
 * NUL-terminated when out_sz > 0. */
size_t hyle_qs_param(const char *qs, const char *key, char *out, size_t out_sz);

#endif