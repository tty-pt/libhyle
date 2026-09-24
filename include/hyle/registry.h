#ifndef HYLE_REGISTRY_H
#define HYLE_REGISTRY_H

/**
 * @file registry.h
 * @brief Source registry and the CRUD / view operations behind it.
 *
 * Registers sources (plain or ordered), writes rows into their corms,
 * and runs filter → sort → paginate queries against them; also the
 * derive-function registry used during index rebuild.
 */

#include <stddef.h>
#include <stdint.h>
#include "ctx.h"
#include "field.h"
#include "query.h"

/* Ordered source flags for hyle_ordered_register */
/**
 * @brief Auto-create the corm record type from the field metadata.
 */
#define HYLE_REGISTRY_AUTO_RECORD  0x01  /* Auto-create corm record from field metadata */

/*
 * Register a source.
 *
 * Creates both the row map (row_hd) and the fields map (fields_hd) internally.
 * record_id: 0 → plain CM_STR value maps; non-zero → typed record corms
 *            created with corm_record_type_id(record_id) + CM_RECORD flag.
 *
 * flags:     Extra corm flags for the row_hd (e.g. CM_SORTED, CM_AINDEX).
 *
 * user:      Opaque pointer stored in the registry.  Retrieve later with
 *            hyle_registry_get_user().  libhyle does not free it.
 *
 * Returns fields_hd on success, 0 on error.
 */
/**
 * @brief Register a source with row and field value maps.
 *
 * Creates the row map (row_hd) and the fields map (fields_hd) for the
 * source, optionally as a typed record corm when record_id is non-zero,
 * and stores the field schema and opaque user pointer for later queries.
 *
 * @param[in] source_id   Unique id identifying the source.
 * @param[in] fields      Array of field descriptors.
 * @param[in] field_count Number of entries in fields.
 * @param[in] record_id   Corm record type id, or 0 for plain maps.
 * @param[in] flags       Extra corm flags for the row map.
 * @param[in] user        Opaque pointer stored by the registry.
 *
 * @return The fields map handle on success, or 0 on error.
 */
unsigned hyle_registry_register(
	const char *source_id,
	const hyle_field_t *fields,
	size_t field_count,
	uint32_t record_id,
	unsigned flags,
	void *user);

/*
 * Add or update a row in a registered source.
 * Writes row_id → "" in row_hd, and row_id:name → value for each field
 * in fields_hd.  names/values are parallel arrays of count entries.
 * Returns 0 on success.
 */
/**
 * @brief Add or update a row in a registered source.
 *
 * Writes the row id into the row map and each name/value pair into the
 * fields map, marking any full-text index dirty.
 *
 * @param[in] source_id Registered source id.
 * @param[in] row_id    Row identifier.
 * @param[in] names     Field names.
 * @param[in] values    Parallel field values.
 * @param[in] count     Number of name/value pairs.
 *
 * @return 0 on success, or -1 when the source or row id is invalid.
 */
int hyle_registry_put(const char *source_id,
	const char *row_id,
	const char **names,
	const char **values,
	size_t count);

/*
 * Delete a row from a registered source.
 * Removes row_id from row_hd and all row_id:* entries from fields_hd.
 */
/**
 * @brief Delete a row and all of its field values from a source.
 *
 * @param[in] source_id Registered source id.
 * @param[in] row_id    Row identifier to remove.
 */
void hyle_registry_del(const char *source_id, const char *row_id);

/*
 * Filter → sort → paginate over the source's live corms.
 * Handles multi-reference position pre-filtering for typed-record sources
 * before delegating to hyle_apply_view.
 * *total_out receives total matching rows before pagination.
 * Caller must call hyle_row_set_free() on *out when done.
 * Returns 0 on success.
 */
/**
 * @brief Run the filter, sort and paginate pipeline over a source.
 *
 * Performs full-text, multi-reference and reference pre-filtering for
 * typed-record sources before delegating to hyle_apply_view(). The
 * caller must call hyle_row_set_free() on *out when done.
 *
 * @param[in]  source_id Registered source id.
 * @param[in]  query     Parsed view state to apply.
 * @param[out] out       Receives the resulting row set.
 * @param[out] total_out Receives the total matching row count, or NULL.
 *
 * @return 0 on success, or -1 when the source is unknown or allocation
 *         fails.
 */
int hyle_registry_query(const char *source_id,
	const hyle_query_t *query,
	hyle_row_set_t *out,
	size_t *total_out);

/* Free the row_hd handle inside a row_set (fields_hd is never owned). */
/**
 * @brief Free the row map handle held by a row set.
 *
 * @param[in] rs Row set to release, or NULL.
 */
void hyle_row_set_free(hyle_row_set_t *rs);

/* ---- Registry accessors ------------------------------------------------- */

/**
 * @brief Look up the row map handle of a registered source.
 *
 * @param[in] source_id Registered source id.
 *
 * @return The row map handle, or 0 when the source is unknown.
 */
unsigned     hyle_registry_get_row_hd(const char *source_id);
/**
 * @brief Look up the fields map handle of a registered source.
 *
 * @param[in] source_id Registered source id.
 *
 * @return The fields map handle, or 0 when the source is unknown.
 */
unsigned     hyle_registry_get_fields_hd(const char *source_id);
/**
 * @brief Retrieve the opaque user pointer stored for a source.
 *
 * @param[in] source_id Registered source id.
 *
 * @return The stored pointer, or NULL when the source is unknown.
 */
void        *hyle_registry_get_user(const char *source_id);
/**
 * @brief Replace the opaque user pointer stored for a source.
 *
 * @param[in] source_id Registered source id.
 * @param[in] user      New pointer to store.
 */
void         hyle_registry_set_user(const char *source_id, void *user);
/**
 * @brief Number of currently registered sources.
 *
 * @return The registry size.
 */
size_t       hyle_registry_count(void);
/**
 * @brief Get the id of the source at a registry index.
 *
 * @param[in] i Registry index.
 *
 * @return The source id, or NULL when i is out of range.
 */
const char  *hyle_registry_id_at(size_t i);
/**
 * @brief Get the field count of a registered source.
 *
 * @param[in] source_id Registered source id.
 *
 * @return The field count, or 0 when the source is unknown.
 */
size_t       hyle_registry_get_field_count(const char *source_id);
/**
 * @brief Get the name of the field at an index of a source.
 *
 * @param[in] source_id Registered source id.
 * @param[in] idx       Field index.
 *
 * @return The field name, or NULL when the source or index is invalid.
 */
const char  *hyle_registry_get_field_name(const char *source_id, size_t idx);
/**
 * @brief Get the type of the field at an index of a source.
 *
 * @param[in] source_id Registered source id.
 * @param[in] idx       Field index.
 *
 * @return The field type, or 0 when the source or index is invalid.
 */
hyle_field_type_t hyle_registry_get_field_type(const char *source_id, size_t idx);

/* ---- FFI helpers (Rust bridge) ----------------------------------------- */

/**
 * @brief Flat row representation for FFI (Rust bridge) consumers.
 */
typedef struct {
	/** Row identifier. */
	const char  *id;
	/** Field names, one per entry in field_values. */
	const char **field_names;
	/** Field values, parallel to field_names. */
	const char **field_values;
	/** Number of name/value pairs. */
	size_t       field_count;
} hyle_ffi_row_t;

/*
 * Convert a row_set to a flat row array.
 * Caller must free with hyle_rows_free(*rows_out, *count_out).
 * Returns 0 on success.
 */
/**
 * @brief Convert a row set into a flat array of FFI rows.
 *
 * @param[in]  rs        Row set to convert.
 * @param[out] rows_out  Receives the row array (call hyle_rows_free()).
 * @param[out] count_out Receives the number of rows.
 *
 * @return 0 on success, or -1 on invalid arguments or allocation failure.
 */
int hyle_row_set_to_rows(const hyle_row_set_t *rs,
	hyle_ffi_row_t **rows_out,
	size_t *count_out);

/**
 * @brief Free an FFI row array returned by hyle_row_set_to_rows().
 *
 * @param[in] rows  Array of rows, or NULL.
 * @param[in] count Number of entries in rows.
 */
void hyle_rows_free(hyle_ffi_row_t *rows, size_t count);

/* ---- Ordered source (positional arrays with pluggable persistence) ------ */

/*
 * Persistence callbacks for ordered sources.
 * load_fn:  called on first access to a partition; should populate items
 *           via hyle_registry_put() with keys "{partition_val}__{NNNN}".
 * save_fn:  called after every mutation; should persist the partition's items.
 * user:     opaque pointer (not freed by libhyle).
 */
/**
 * @brief Load callback for ordered sources.
 *
 * @param[in] source_id     Registered source id.
 * @param[in] partition_val Partition whose items must be loaded.
 * @param[in] fields_hd     Fields map handle to populate.
 * @param[in] user          Opaque pointer passed at registration.
 *
 * @return 0 on success, non-zero on failure.
 */
typedef int (*hyle_persist_load_fn)(const char *source_id,
	const char *partition_val, unsigned fields_hd, void *user);
/**
 * @brief Save callback for ordered sources.
 *
 * @param[in] source_id     Registered source id.
 * @param[in] partition_val Partition whose items must be persisted.
 * @param[in] fields_hd     Fields map handle holding the items.
 * @param[in] user          Opaque pointer passed at registration.
 *
 * @return 0 on success, non-zero on failure.
 */
typedef int (*hyle_persist_save_fn)(const char *source_id,
	const char *partition_val, unsigned fields_hd, void *user);

/*
 * Register an ordered (partitioned positional array) source.
 *
 * partition_field:  field name that scopes items to a partition (e.g. "sb").
 *                   Items are stored with keys "{partition_val}__{NNNN}".
 * load_fn / save_fn:  custom persistence callbacks (may be NULL).
 * persist_user:       passed to load_fn/save_fn.
 *
 * Returns fields_hd on success, 0 on error.
 */
/**
 * @brief Register an ordered, partitioned positional-array source.
 *
 * Items within a partition are addressed by zero-based position and
 * stored under "{partition_val}__{NNNN}" keys. When
 * HYLE_REGISTRY_AUTO_RECORD is set and record_id is 0, the record type
 * is derived from the field metadata.
 *
 * @param[in] source_id       Unique id identifying the source.
 * @param[in] fields          Array of field descriptors.
 * @param[in] field_count     Number of entries in fields.
 * @param[in] partition_field Field scoping items to a partition.
 * @param[in] record_id       Corm record type id, or 0 for plain maps.
 * @param[in] flags           Extra flags (e.g. HYLE_REGISTRY_AUTO_RECORD).
 * @param[in] load_fn         Load callback, or NULL.
 * @param[in] save_fn         Save callback, or NULL.
 * @param[in] persist_user    Opaque pointer passed to the callbacks.
 *
 * @return The fields map handle on success, or 0 on error.
 */
unsigned hyle_ordered_register(
	const char *source_id,
	const hyle_field_t *fields, size_t field_count,
	const char *partition_field,
	uint32_t record_id, unsigned flags,
	hyle_persist_load_fn load_fn,
	hyle_persist_save_fn save_fn,
	void *persist_user);

/* Number of items in partition partition_val. */
/**
 * @brief Count the items in an ordered partition.
 *
 * @param[in] source_id     Registered ordered source id.
 * @param[in] partition_val Partition to count.
 *
 * @return The item count (zero when the partition is unloaded or absent).
 */
int hyle_ordered_count(const char *source_id,
	const char *partition_val);

/*
 * Get the key for item at position pos in partition partition_val.
 * Returns a pointer to a static buffer (valid until next ordered key_at call).
 * Returns NULL if the item doesn't exist.
 */
/**
 * @brief Get the storage key of the item at a partition position.
 *
 * @param[in] source_id     Registered ordered source id.
 * @param[in] partition_val Partition holding the item.
 * @param[in] pos           Zero-based item position.
 *
 * @return The "{partition_val}__{NNNN}" key in a static buffer (valid
 *         until the next call), or NULL when the item does not exist.
 */
const char *hyle_ordered_key_at(const char *source_id,
	const char *partition_val, int pos);

/*
 * Append an item to the end of a partition.  Triggers save.
 * names/values are parallel arrays of count entries.
 * Returns 0 on success.
 */
/**
 * @brief Append an item to the end of a partition and trigger save.
 *
 * @param[in] source_id     Registered ordered source id.
 * @param[in] partition_val Partition to append to.
 * @param[in] names         Field names.
 * @param[in] values        Parallel field values.
 * @param[in] count         Number of name/value pairs.
 *
 * @return 0 on success, or -1 when the partition cannot be loaded.
 */
int hyle_ordered_append(const char *source_id,
	const char *partition_val,
	const char **names, const char **values, size_t count);

/*
 * Insert an item at position pos.  Shifts items [pos..end] forward.
 * Triggers save.  Returns 0 on success.
 */
/**
 * @brief Insert an item at a partition position, shifting later items up.
 *
 * @param[in] source_id     Registered ordered source id.
 * @param[in] partition_val Partition holding the item.
 * @param[in] pos           Zero-based insertion position.
 * @param[in] names         Field names.
 * @param[in] values        Parallel field values.
 * @param[in] count         Number of name/value pairs.
 *
 * @return 0 on success, or -1 when the position is out of range.
 */
int hyle_ordered_insert_at(const char *source_id,
	const char *partition_val, int pos,
	const char **names, const char **values, size_t count);

/* Remove the item at position pos.  Shifts items [pos+1..end] backward.
 * Triggers save. */
/**
 * @brief Remove the item at a partition position and trigger save.
 *
 * @param[in] source_id     Registered ordered source id.
 * @param[in] partition_val Partition holding the item.
 * @param[in] pos           Zero-based item position to remove.
 */
void hyle_ordered_remove_at(const char *source_id,
	const char *partition_val, int pos);

/* Remove all items in the partition.  Triggers save. */
/**
 * @brief Remove every item in a partition and trigger save.
 *
 * @param[in] source_id     Registered ordered source id.
 * @param[in] partition_val Partition to clear.
 */
void hyle_ordered_clear(const char *source_id,
	const char *partition_val);

/*
 * Explicitly trigger the save callback for a partition.
 * Useful after modifying fields via hyle_registry_put() on an ordered
 * source's keys.
 */
/**
 * @brief Explicitly trigger the save callback for a partition.
 *
 * @param[in] source_id     Registered ordered source id.
 * @param[in] partition_val Partition to persist.
 */
void hyle_ordered_save(const char *source_id,
	const char *partition_val);

/* ---- Derive field registry ---------------------------------------------- */
/*
 * Function signature for derived field providers.
 * Called during index rebuild to get the searchable value for a derived field.
 *
 * def:     Source definition (opaque, retrieved via hyle_registry_get_user())
 * row_id:  Row identifier
 * field_name: Name of the derived field being queried
 *
 * Returns: Pointer to static/thread-local buffer (caller must not free),
 *          or NULL/empty string if no value.
 */
/**
 * @brief Provider signature for derived field values.
 *
 * @param[in] def        Opaque source definition (user pointer).
 * @param[in] row_id     Row identifier.
 * @param[in] field_name Name of the derived field being queried.
 * @param[in] user       Opaque user data (unused).
 *
 * @return Pointer to a static/thread-local value buffer (caller must not
 *         free), or NULL when there is no value.
 */
typedef const char *(*hyle_derive_fn_t)(
	const void *def,
	const char *row_id,
	const char *field_name,
	void *user);

/*
 * Register a derive provider for a source.
 * Called by modules during xy_install() after source registration.
 *
 * derive_key:  Key matching hyle_field_t.derive_key (e.g. "song.lyrics_from_data")
 * fn:          Provider function
 * user:        Opaque user data passed to fn
 */
/**
 * @brief Register a derive provider under a derive key.
 *
 * @param[in] derive_key Key matching hyle_field_t.derive_key.
 * @param[in] fn         Provider function.
 * @param[in] user       Opaque user data passed to fn.
 *
 * @return 0 on success, or -1 when the provider table is full.
 */
int hyle_derive_register(const char *derive_key, hyle_derive_fn_t fn, void *user);

/*
 * Lookup and call a derive provider.
 * Called internally during index rebuild.
 */
/**
 * @brief Look up and invoke the provider for a derive key.
 *
 * @param[in] def        Opaque source definition passed to the provider.
 * @param[in] derive_key Key registering the provider to call.
 * @param[in] row_id     Row identifier.
 * @param[in] field_name Name of the derived field being queried.
 * @param[in] user       Opaque user data (unused).
 *
 * @return The provider's value, or NULL when no provider is registered.
 */
const char *hyle_derive_call(const void *def, const char *derive_key, const char *row_id, const char *field_name, void *user);

#endif
