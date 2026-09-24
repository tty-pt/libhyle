#ifndef HYLE_PURIFY_H
#define HYLE_PURIFY_H

/**
 * @file purify.h
 * @brief Row-level field validation and error collection.
 *
 * Validates a row's values against the field schema and reports
 * per-field violations without touching storage.
 */

#include <stddef.h>
#include <stdint.h>
#include "field.h"

/**
 * @brief One validation violation reported for a row field.
 */
typedef struct {
	/** Name of the field that failed validation. */
	const char *field;
	/** Machine-readable rule id (e.g. "required", "min", "pattern"). */
	const char *rule;
	/** Human-readable description of the violation. */
	const char *message;
} hyle_purify_error_t;

/**
 * @brief Validate a row's values against a field schema.
 *
 * Checks required presence, integer min/max, string length bounds and
 * POSIX extended regex patterns without touching storage. On failure the
 * violations are returned through errors_out; release them with
 * hyle_purify_errors_free().
 *
 * @param[in]  fields           Array of field descriptors.
 * @param[in]  field_count      Number of entries in fields.
 * @param[in]  values           Parallel array of raw string values, or NULL.
 * @param[out] errors_out       Receives the violation array, or NULL.
 * @param[out] error_count_out  Receives the number of violations, or NULL.
 *
 * @return 1 when the row failed validation (or an error was raised),
 *         0 when the row is valid or the arguments are invalid.
 */
int hyle_purify_row(
	const hyle_field_t *fields,
	size_t field_count,
	const char **values,
	hyle_purify_error_t **errors_out,
	size_t *error_count_out);

/**
 * @brief Free the error array and its message strings.
 *
 * @param[in] errors      Array returned by hyle_purify_row(), or NULL.
 * @param[in] error_count Number of entries in errors.
 */
void hyle_purify_errors_free(
	hyle_purify_error_t *errors,
	size_t error_count);

#endif
