#ifndef HYLE_BLUEPRINT_H
#define HYLE_BLUEPRINT_H

/**
 * @file blueprint.h
 * @brief Manifest construction over one or more sources.
 *
 * A blueprint composes a base source with select/filter/pagination
 * options to produce a resolved manifest for rendering.
 */

#include <stddef.h>
#include <stdint.h>
#include "field.h"
#include "query.h"

/**
 * @brief Descriptor for one source in a blueprint, resolved against a query.
 */
typedef struct {
	/** Source identifier, as registered with hyle_registry_register(). */
	const char *id;
	/** Name of the primary key field (typically "id"). */
	const char *key_field;
	/** Array of field descriptors for the source. */
	const hyle_field_t *fields;
	/** Number of entries in fields. */
	size_t field_count;
	/** Corm handle of the source's stored field values. */
	uint32_t fields_hd;
} hyle_blueprint_schema_t;

/**
 * @brief Collection of source schemas forming a manifest blueprint.
 */
typedef struct {
	/** Array of source descriptors. */
	hyle_blueprint_schema_t *sources;
	/** Number of entries in sources. */
	size_t source_count;
} hyle_blueprint_t;

/**
 * @brief Resolved per-field filter in a manifest.
 */
typedef struct {
	/** Field name to filter on. */
	const char *field;
	/** Filter value to match. */
	const char *value;
	/** Non-zero when the field type is a reference. */
	int is_reference;
} hyle_manifest_filter_t;

/**
 * @brief Resolved rendering request derived from a blueprint and a query.
 */
typedef struct {
	/** Id of the source to render as the base rows. */
	const char *base_source;
	/** Field names requested for selection (include). */
	const char **select;
	/** Number of entries in select. */
	size_t select_count;
	/** Resolved per-field filters to apply. */
	hyle_manifest_filter_t *filter_list;
	/** Number of entries in filter_list. */
	size_t filter_count;
	/** Free-text search term, or NULL when absent. */
	const char *query;
	/** Field name to sort by, or NULL for no sort. */
	const char *sort_field;
	/** Non-zero for ascending sort order. */
	int sort_asc;
	/** One-based page number; 0 disables pagination. */
	uint32_t page;
	/** Rows per page; 0 disables pagination. */
	uint32_t per_page;
	/** Target source ids to load for reference-filter lookups. */
	const char **lookups;
	/** Number of entries in lookups. */
	size_t lookup_count;
	/** Target source ids to inline for reference-field includes. */
	const char **inlines;
	/** Number of entries in inlines. */
	size_t inline_count;
} hyle_manifest_t;

/**
 * @brief Resolve a blueprint source into a rendering manifest.
 *
 * Finds the schema in bp that matches source_id and copies the query's
 * filter, sort, page and include settings into out. Reference filters
 * register their target source in lookups; reference includes register
 * their target source in inlines.
 *
 * @param[in]  bp        Blueprint holding the source schemas.
 * @param[in]  source_id Id of the source to resolve.
 * @param[in]  query     Parsed query carrying filters/sort/page/include.
 * @param[out] out       Manifest to fill; clear with hyle_manifest_clear().
 *
 * @return 0 on success, or -1 when the source or any referenced field is
 *         unknown or an allocation fails.
 */
int hyle_blueprint_manifest(
	const hyle_blueprint_t *bp,
	const char *source_id,
	const hyle_query_t *query,
	hyle_manifest_t *out);

/**
 * @brief Free the arrays owned by a manifest and zero it.
 *
 * @param[in] m Manifest to release, or NULL.
 */
void hyle_manifest_clear(hyle_manifest_t *m);

#endif
