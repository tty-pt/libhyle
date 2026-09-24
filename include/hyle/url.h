#ifndef HYLE_URL_H
#define HYLE_URL_H

/**
 * @file url.h
 * @brief Query-string parameter extraction without input mutation.
 */

#include <stddef.h>

/**
 * @brief Extract a query-string parameter without modifying the input.
 *
 * Finds `key` in the query string and URL-decodes its value (%XX hex
 * escapes and '+' as space) into out.
 *
 * @param[in]  qs     Query string to scan.
 * @param[in]  key    Parameter name to look up.
 * @param[out] out    Destination buffer for the decoded value.
 * @param[in]  out_sz Capacity of out.
 * @return Bytes written; 0 when the key is absent or an argument is invalid.
 *         out is always NUL-terminated when out_sz > 0.
 */
size_t hyle_qs_param(const char *qs, const char *key, char *out, size_t out_sz);

#endif