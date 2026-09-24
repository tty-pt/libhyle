#ifndef HYLE_CTX_H
#define HYLE_CTX_H

/**
 * @file ctx.h
 * @brief Allocation context for the hyle data engine.
 *
 * Tracks the string/array/map pools that back values and ids created
 * within a dataset context.
 */

#include <stdint.h>

/**
 * @brief Allocation context for the hyle data engine.
 *
 * Holds the corm pool handles that back values and row ids, plus a
 * scratch pool for intermediate view results.
 */
typedef struct hyle_ctx_t {
	/** Corm handle of the pool backing string values. */
	unsigned string_pool;
	/** Corm handle of the pool backing array values. */
	unsigned array_pool;
	/** Corm handle of the pool backing map values. */
	unsigned map_pool;
	/** Corm handle of the scratch pool for intermediate results. */
	unsigned scratch;
	/** Monotonic id counter used to mint new pool keys. */
	uint32_t next_id;
} hyle_ctx_t;

/**
 * @brief Create and initialize a new allocation context.
 *
 * Opens the string, array, map and scratch corm pools backing the
 * context. The caller owns the returned context, and must release it
 * with hyle_ctx_free().
 *
 * @return A new context, or NULL when out of memory.
 */
hyle_ctx_t *hyle_ctx_new(void);

/**
 * @brief Close a context's pools and free the context itself.
 *
 * @param[in] ctx Context to release, or NULL.
 */
void hyle_ctx_free(hyle_ctx_t *ctx);

#endif
