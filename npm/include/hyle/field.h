#ifndef HYLE_FIELD_H
#define HYLE_FIELD_H

/**
 * @file field.h
 * @brief Field descriptors and validation metadata.
 *
 * Declares the field types a hyle source can expose, plus the per-field
 * constraints used by filtering, purification and schema hints.
 */

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Storage and matching category of a hyle field.
 */
typedef enum {
	/** Plain string value. */
	HYLE_FIELD_STRING = 0,
	/** Integer value. */
	HYLE_FIELD_INT,
	/** Boolean value. */
	HYLE_FIELD_BOOL,
	/** String value that may be absent. */
	HYLE_FIELD_NULLABLE_STRING,
	/** Single reference to a row of another source. */
	HYLE_FIELD_REFERENCE,
	/** Newline-separated list of references to another source. */
	HYLE_FIELD_MULTI_REFERENCE,
	/** Virtual inverse relation over a foreign reference field. */
	HYLE_FIELD_INVERSE,
	/** Derived value computed by a registered derive provider. */
	HYLE_FIELD_DERIVED = 99,
} hyle_field_type_t;

/**
 * @brief Per-field descriptor and validation constraints.
 */
typedef struct {
	/** Field name. */
	const char *name;
	/** Storage and matching category of the field. */
	hyle_field_type_t type;
	/** Non-zero when the field may be written by clients. */
	int writable;
	/** Source id referenced by reference-typed fields. */
	const char *target_source;
	/** Name of the inverse field for HYLE_FIELD_INVERSE. */
	const char *inverse_name;
	/** Non-zero when a value is mandatory for the field. */
	int required;
	/** Inclusive lower bound for integer fields. */
	int64_t min;
	/** Inclusive upper bound for integer fields. */
	int64_t max;
	/** Minimum length for string fields. */
	size_t min_length;
	/** Maximum length for string fields. */
	size_t max_length;
	/** POSIX extended regex that string values must match. */
	const char *pattern;
	/** Non-zero when the field participates in full-text search. */
	int searchable;
	int combine; /* 0 = OR (default), 1 = AND within-field */
	/** Key that selects a derive provider for derived fields. */
	const char *derive_key;
} hyle_field_t;

/**
 * @brief Find a field descriptor by name.
 *
 * @param[in] fields      Array of field descriptors.
 * @param[in] field_count Number of entries in fields.
 * @param[in] name        Field name to look up.
 *
 * @return Pointer to the matching descriptor, or NULL when not found.
 */
const hyle_field_t *hyle_field_by_name(
	const hyle_field_t *fields,
	size_t field_count,
	const char *name);

/**
 * @brief Test whether a field type is a reference category.
 *
 * Returns non-zero for HYLE_FIELD_REFERENCE, HYLE_FIELD_MULTI_REFERENCE
 * and HYLE_FIELD_INVERSE.
 *
 * @param[in] type Field type to test.
 *
 * @return 1 for reference categories, 0 otherwise.
 */
int hyle_field_is_reference(hyle_field_type_t type);

#endif
