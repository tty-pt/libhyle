#ifndef HYLE_QUERY_H
#define HYLE_QUERY_H

/**
 * @file query.h
 * @brief URL query parsing and the filter/sort/paginate pipeline.
 *
 * hyle_query_t carries parsed sort/page/filter state; the helpers below
 * apply it to a row set (accent-sensitive matching, corm-backed filter).
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "ctx.h"
#include "field.h"

/**
 * @brief Per-field filter as parsed from the URL query string.
 */
typedef struct {
	/** Field name being filtered. */
	const char *field;
	/** Filter value to match. */
	const char *value;
	const char *op; /* NULL, "and", or "or" — from <field>_op URL param */
} hyle_field_filter_t;

/**
 * @brief Parsed view state: sort, page and filter options.
 */
typedef struct {
	/** Field name to sort by, or NULL for no sort. */
	const char *sort_field;
	/** Non-zero for ascending sort order. */
	bool sort_asc;
	/** One-based page number; 0 disables pagination. */
	uint32_t page;
	/** Rows per page; 0 disables pagination. */
	uint32_t per_page;
	/** Free-text search term, or NULL when absent. */
	const char *q;
	/** Per-field filters to apply (ANDed across fields). */
	hyle_field_filter_t *filters;
	/** Number of entries in filters. */
	unsigned filter_count;
	/** Field names requested for inclusion. */
	const char **include;
	/** Number of entries in include. */
	unsigned include_count;
} hyle_query_t;

/**
 * @brief Handle pair identifying a set of rows and its field values.
 */
typedef struct {
	/** Corm handle of the row id set (owned by the producer). */
	unsigned row_hd;
	/** Corm handle of the field values map (never owned). */
	unsigned fields_hd;
} hyle_row_set_t;

/*
 * Parse URL-encoded query string into a hyle_query_t.
 * Modifies query_str in place (null-terminates tokens).
 * Caller must keep query_str alive while the query_t is in use.
 * Allocates ->filters and ->include arrays via malloc; caller
 * must call hyle_query_clear() to free them.
 */
/**
 * @brief Parse a URL-encoded query string into a hyle_query_t.
 *
 * Recognizes sort, page, per_page, q and include, treats "<field>_op"
 * as a within-field combination override, and turns every other
 * key=value pair into a filter. query_str is modified in place and the
 * parsed pointers reference its tokens, so it must outlive the query.
 *
 * @param[in]  query_str URL-encoded query string (modified in place).
 * @param[out] out       Query to fill; clear with hyle_query_clear().
 *
 * @return 0 on success, or -1 when an allocation fails.
 */
int hyle_parse_query(char *query_str, hyle_query_t *out);

/* Free dynamically allocated members of a parsed query. */
/**
 * @brief Free the filter and include arrays owned by a parsed query.
 *
 * @param[in] q Query to release, or NULL.
 */
void hyle_query_clear(hyle_query_t *q);

/* Case-insensitive, accent-sensitive substring match (stoma_fold). */
/**
 * @brief Case-insensitive, accent-sensitive substring match.
 *
 * @param[in] str String to search.
 * @param[in] sub Substring to look for.
 *
 * @return 1 when sub occurs in str (case-folded), 0 otherwise.
 */
int hyle_ci_substr(const char *str, const char *sub);

/* Case-insensitive, accent-sensitive substring match with punctuation
 * normalization (Google-like): replaces punctuation except apostrophes and
 * hyphens within words with spaces, collapses whitespace. */
/**
 * @brief Case-insensitive substring match with punctuation normalization.
 *
 * Most punctuation is replaced with spaces, apostrophes are stripped,
 * and whitespace is collapsed before a folded substring match, so
 * "well-known" matches "well known" and "don't" matches "dont".
 *
 * @param[in] str String to search.
 * @param[in] sub Substring to look for.
 *
 * @return 1 when sub matches str after normalization, 0 otherwise.
 */
int hyle_ci_substr_punct(const char *str, const char *sub);

/*
 * Filter rows: emit matching row IDs to output->row_hd.
 * Edits: output->row_hd written, output->fields_hd set to input->fields_hd.
 * q != NULL     → full-text search across id + all string fields.
 * filters       → per-field match (AND).
 * fields/field_count → optional field schema; when provided,
 *   HYLE_FIELD_MULTI_REFERENCE fields use newline-boundary token matching
 *   instead of substring match.  Pass NULL/0 for pure substring behaviour.
 */
/**
 * @brief Emit the rows in input that match q and all filters.
 *
 * A NULL/empty q passes all rows; per-field filters are ANDed and match
 * case-insensitively. When the field schema is supplied, multi-reference
 * fields match on whole newline-delimited tokens instead of substrings.
 *
 * @param[in]  ctx          Context (unused, may be NULL).
 * @param[in]  input        Source row set.
 * @param[in]  q            Free-text term, or NULL for no search.
 * @param[in]  filters      Per-field filters, or NULL.
 * @param[in]  filter_count Number of entries in filters.
 * @param[in]  fields       Field schema for multi-reference token matching.
 * @param[in]  field_count  Number of entries in fields.
 * @param[out] output       Receives the matching row set.
 */
void hyle_filter_rows(hyle_ctx_t *ctx,
	const hyle_row_set_t *input,
	const char *q,
	const hyle_field_filter_t *filters,
	unsigned filter_count,
	const hyle_field_t *fields,
	size_t field_count,
	hyle_row_set_t *output);

/*
 * Sort rows by sort_field. Auto-detects numeric vs string.
 * sort_field == NULL → passthrough.
 * output->fields_hd set to input->fields_hd.
 */
/**
 * @brief Sort a row set by a field value (numeric when all numeric).
 *
 * @param[in]  ctx        Context (unused, may be NULL).
 * @param[in]  input      Source row set.
 * @param[in]  sort_field Field to sort by, or NULL for passthrough.
 * @param[in]  sort_asc   Non-zero for ascending order.
 * @param[out] output     Receives the sorted row set.
 */
void hyle_sort_rows(hyle_ctx_t *ctx,
	const hyle_row_set_t *input,
	const char *sort_field,
	bool sort_asc,
	hyle_row_set_t *output);

/*
 * Paginate: emit only the requested page.
 * page=0 or per_page=0 → passthrough (all rows).
 * *total_out receives total matching rows before pagination.
 */
/**
 * @brief Emit only the requested page of a row set.
 *
 * @param[in]  ctx        Context (unused, may be NULL).
 * @param[in]  input      Source row set.
 * @param[in]  page       One-based page number.
 * @param[in]  per_page   Rows per page.
 * @param[out] output     Receives the page's rows.
 * @param[out] total_out  Receives the total row count, or NULL.
 */
void hyle_paginate(hyle_ctx_t *ctx,
	const hyle_row_set_t *input,
	uint32_t page,
	uint32_t per_page,
	hyle_row_set_t *output,
	uint32_t *total_out);

/*
 * Combined pipeline: filter → sort → paginate in one call.
 * Intermediate results use ctx->scratch.
 * fields/field_count forwarded to hyle_filter_rows (may be NULL/0).
 */
/**
 * @brief Apply the filter, sort and paginate pipeline in one call.
 *
 * @param[in]  ctx         Context used for intermediate results.
 * @param[in]  input       Source row set.
 * @param[in]  query       Parsed view state to apply.
 * @param[in]  fields      Field schema, forwarded to hyle_filter_rows().
 * @param[in]  field_count Number of entries in fields.
 * @param[out] output      Receives the resulting row set.
 * @param[out] total_out   Receives the total matching row count, or NULL.
 */
void hyle_apply_view(hyle_ctx_t *ctx,
	const hyle_row_set_t *input,
	const hyle_query_t *query,
	const hyle_field_t *fields,
	size_t field_count,
	hyle_row_set_t *output,
	uint32_t *total_out);

#endif
