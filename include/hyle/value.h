#ifndef HYLE_VALUE_H
#define HYLE_VALUE_H

/**
 * @file value.h
 * @brief 16-byte hyle value type and JSON serialization.
 *
 * Values allocate against a hyle_ctx_t pool; accessors and JSON output
 * live here alongside the boxed constructors.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "ctx.h"

typedef enum {
	/** Null value. */
	HYLE_NULL = 0,
	/** Boolean value. */
	HYLE_BOOL,
	/** 64-bit signed integer value. */
	HYLE_INT,
	/** Double-precision floating point value. */
	HYLE_FLOAT,
	/** Pool-allocated string value. */
	HYLE_STRING,
	/** Ordered array of values. */
	HYLE_ARRAY,
	/** String-keyed map of values. */
	HYLE_MAP,
} hyle_val_type_t;

/**
 * @brief 16-byte hyle value.
 *
 * Small inline union of the primitive storage types; container values
 * (strings, arrays, maps) are handles into the context pool.
 */
typedef struct {
	/** Value category. */
	hyle_val_type_t type;
	/** Ownership/representation flags. */
	uint8_t flags;
	/** Reserved padding. */
	uint8_t _pad[3];
	/** Inline payload by type. */
	union {
		bool b;
		int64_t i;
		double f;
		uint32_t hd;
	};
} hyle_val_t;

_Static_assert(sizeof(hyle_val_t) == 16, "hyle_val_t must be 16 bytes");

/**
 * @brief Identifies one stored row in a hyle source.
 */
typedef struct {
	/** Context pool that owns the row. */
	hyle_ctx_t *ctx;
	/** Row handle within the context. */
	uint32_t hd;
} hyle_row_t;

/**
 * @brief Construct a null value.
 * @return A value of type HYLE_NULL.
 */
hyle_val_t hyle_val_null(void);

/**
 * @brief Construct a boolean value.
 * @param[in] b Source boolean.
 * @return A value of type HYLE_BOOL.
 */
hyle_val_t hyle_val_bool(bool b);

/**
 * @brief Construct a 64-bit signed integer value.
 * @param[in] i Source integer.
 * @return A value of type HYLE_INT.
 */
hyle_val_t hyle_val_int(int64_t i);

/**
 * @brief Construct a double-precision float value.
 * @param[in] f Source float.
 * @return A value of type HYLE_FLOAT.
 */
hyle_val_t hyle_val_float(double f);

/**
 * @brief Construct a pooled string value from a C string.
 * @param[in] ctx Context pool to allocate from.
 * @param[in] s   NUL-terminated source string.
 * @return A value of type HYLE_STRING.
 */
hyle_val_t hyle_val_string(hyle_ctx_t *ctx, const char *s);

/**
 * @brief Construct an empty array value.
 * @param[in] ctx Context pool to allocate from.
 * @return A value of type HYLE_ARRAY.
 */
hyle_val_t hyle_val_array(hyle_ctx_t *ctx);

/**
 * @brief Construct an empty map value.
 * @param[in] ctx Context pool to allocate from.
 * @return A value of type HYLE_MAP.
 */
hyle_val_t hyle_val_map(hyle_ctx_t *ctx);

/**
 * @brief Read back the characters of a string value.
 * @param[in] ctx Context pool that owns the value.
 * @param[in] v   String value.
 * @return NUL-terminated string (pool owned) or NULL for non-strings.
 */
const char *hyle_val_string_get(hyle_ctx_t *ctx, hyle_val_t v);

/**
 * @brief Append an element to an array value.
 * @param[in] ctx Context pool that owns the values.
 * @param[in] arr Array value.
 * @param[in] elem Element to append (copied into the pool).
 */
void hyle_val_array_push(hyle_ctx_t *ctx, hyle_val_t arr, hyle_val_t elem);

/**
 * @brief Read one element of an array value.
 * @param[in] ctx Context pool that owns the values.
 * @param[in] arr Array value.
 * @param[in] idx Element index.
 * @return Element value, or HYLE_NULL when out of range.
 */
hyle_val_t hyle_val_array_get(hyle_ctx_t *ctx, hyle_val_t arr, size_t idx);

/**
 * @brief Number of elements in an array value.
 * @param[in] ctx Context pool that owns the value.
 * @param[in] arr Array value.
 * @return Element count.
 */
size_t hyle_val_array_len(hyle_ctx_t *ctx, hyle_val_t arr);

/**
 * @brief Set a key in a map value.
 * @param[in] ctx Context pool that owns the values.
 * @param[in] map Map value.
 * @param[in] key NUL-terminated key string.
 * @param[in] val Value to store under key (copied into the pool).
 */
void hyle_val_map_set(hyle_ctx_t *ctx, hyle_val_t map, const char *key, hyle_val_t val);

/**
 * @brief Read one entry of a map value.
 * @param[in] ctx Context pool that owns the values.
 * @param[in] map Map value.
 * @param[in] key NUL-terminated key string.
 * @return Entry value, or HYLE_NULL when absent.
 */
hyle_val_t hyle_val_map_get(hyle_ctx_t *ctx, hyle_val_t map, const char *key);

/**
 * @brief Serialize a value to JSON.
 * @param[in] ctx Context pool that owns the value.
 * @param[in] v   Value to serialize.
 * @return malloc'd JSON string; caller frees with free().
 */
char *hyle_val_to_json(hyle_ctx_t *ctx, hyle_val_t v);

#endif
