#ifndef HYLE_SCHEMA_H
#define HYLE_SCHEMA_H

/**
 * @file schema.h
 * @brief Canonical, framework-neutral schema descriptors.
 *
 * hyle_schema_desc_t is the storage-agnostic field descriptor shared by
 * hyle and bud; the FIELD_* macros generate them from struct layouts.
 */

#include <stddef.h>
#include "field.h"

/* ── Hyle Schema Descriptor Kinds (UI / Serialization Mode) ───── */
/** Standard record field, included in state JSON. */
#define HYLE_KIND_RECORD      0
/** Record field, excluded from state JSON. */
#define HYLE_KIND_EXCLUDE     1
/** Reference field, resolving IDs to display names. */
#define HYLE_KIND_REF_DISPLAY 2
/** Computed integer overlay. */
#define HYLE_KIND_OVERLAY_INT 3
/** Computed string overlay. */
#define HYLE_KIND_OVERLAY_STR 4
/** Inverse virtual relation. */
#define HYLE_KIND_INVERSE     5

/* ── Hyle Storage / Corm Types ────────────────────────────────── */
/** Corm string storage type. */
#define HYLE_CM_STR           2
/** Corm single-reference storage type. */
#define HYLE_CM_REFERENCE     6
/** Corm multi-reference storage type. */
#define HYLE_CM_MULTI_REF     7
/** Corm variable-length string storage type. */
#define HYLE_CM_VSTR          8

/* Backward compatibility aliases */
#ifndef BUD_RECORD
#define BUD_RECORD HYLE_KIND_RECORD
#define BUD_EXCLUDE HYLE_KIND_EXCLUDE
#define BUD_REF_DISPLAY HYLE_KIND_REF_DISPLAY
#define BUD_OVERLAY_INT HYLE_KIND_OVERLAY_INT
#define BUD_OVERLAY_STR HYLE_KIND_OVERLAY_STR
#define BUD_INVERSE HYLE_KIND_INVERSE
#endif
#ifndef BUD_CM_STR
#define BUD_CM_STR HYLE_CM_STR
#define BUD_CM_VSTR HYLE_CM_VSTR
#define BUD_CM_REFERENCE HYLE_CM_REFERENCE
#define BUD_CM_MULTI_REF HYLE_CM_MULTI_REF
#endif

/* ── Canonical Hyle Schema Descriptor ─────────────────────────── */
/**
 * @brief Framework-neutral, pure-C schema descriptor for one field.
 *
 * Layout contract: the first 5 fields match bud_field_desc_t so the two
 * can be stride-cast with zero copy. The FIELD_* macros below build
 * these from struct layouts.
 */
typedef struct hyle_schema_desc {
	/** Field key (struct member name, JSON name). */
	const char *key;
	/** Byte offset of the member in its owning struct (0 if virtual). */
	size_t offset;
	/** Byte size of the member (0 if virtual). */
	size_t size;
	/** Non-zero when the field stores an integer. */
	int is_int;
	/** HYLE_KIND_* serialization/UI mode. */
	int kind;
	/** 0 = scalar, 1 = collection/array. */
	int is_array;
	/** Corm type id (HYLE_CM_*). */
	int qm_type;
	/** Field category (source_type is a legacy alias). */
	union {
		hyle_field_type_t type;
		int source_type;
	};
	/** Non-zero when the field may be written by clients. */
	int writable;
	/** Non-zero when a value is mandatory. */
	int required;
	/** Minimum string length (0 = unconstrained). */
	size_t min_length;
	/** Source id referenced by reference fields. */
	const char *ref_source;
	/** Inverse field name for inverse relations. */
	const char *ref_inverse;
	/** Non-zero when the field lives in meta rather than record state. */
	int in_meta;
	/** Attached file name, or NULL. */
	const char *file;
	/** Display/filter style hint, or NULL. */
	const char *filter_style;
	/** Filter matching mode, or NULL. */
	const char *filter_mode;
	/** Key selecting a derive provider for derived fields, or NULL. */
	const char *derive_key;
	/** Non-zero when new entries may be added through this field. */
	int allow_add;
} hyle_schema_desc_t;

/** Alias for hyle_schema_desc_t. */
typedef struct hyle_schema_desc source_desc_t;

/* ── Member Size and Offset Helpers ───────────────────────────── */
/** Size in bytes of a struct member (\p mb of type \p st). */
#ifndef FIELD_SIZE
#define FIELD_SIZE(st, mb) sizeof(((st *)0)->mb)
#endif
/** Byte offset of a struct member (\p mb of type \p st). */
#ifndef FIELD_OFFSET
#define FIELD_OFFSET(st, mb) offsetof(st, mb)
#endif

/* ── Base Field Property Generator Helpers ─────────────────────── */
/** @brief Text-field initializer body used by FIELD_TEXT. */
#define _FIELD_TEXT(name, st, ...)                                             \
	.key = #name,                                                          \
	.offset = offsetof(st, name),                                          \
	.size = sizeof(((st *)0)->name),                                       \
	.is_int = 0,                                                           \
	.kind = HYLE_KIND_RECORD,                                              \
	.qm_type = HYLE_CM_STR,                                                \
	.type = HYLE_FIELD_STRING,                                              \
	.writable = 1,                                                         \
	##__VA_ARGS__

/** @brief Integer-field initializer body used by FIELD_INT. */
#define _FIELD_INT(name, st, ...)                                              \
	.key = #name,                                                          \
	.offset = offsetof(st, name),                                          \
	.size = sizeof(int),                                                   \
	.is_int = 1,                                                           \
	.kind = HYLE_KIND_RECORD,                                              \
	.type = HYLE_FIELD_INT,                                                 \
	.writable = 1,                                                         \
	##__VA_ARGS__

/** @brief Boolean-field initializer body used by FIELD_BOOL. */
#define _FIELD_BOOL(name, st, ...)                                             \
	.key = #name,                                                          \
	.offset = offsetof(st, name),                                          \
	.size = sizeof(int),                                                   \
	.is_int = 0,                                                           \
	.kind = HYLE_KIND_RECORD,                                              \
	.type = HYLE_FIELD_BOOL,                                                \
	.writable = 1,                                                         \
	##__VA_ARGS__

/** @brief Reference-field initializer body used by FIELD_REF. */
#define _FIELD_REF(name, st, target, ...)                                      \
	.key = #name,                                                          \
	.offset = offsetof(st, name),                                          \
	.size = sizeof(((st *)0)->name),                                       \
	.is_int = 0,                                                           \
	.kind = HYLE_KIND_REF_DISPLAY,                                         \
	.qm_type = HYLE_CM_REFERENCE,                                          \
	.type = HYLE_FIELD_REFERENCE,                                           \
	.writable = 1,                                                         \
	.ref_source = target,                                                  \
	.file = #name,                                                         \
	##__VA_ARGS__

/* ── Modern Ergonomic Schema Macros ───────────────────────────── */

/** @brief String/text record field descriptor. */
#define FIELD_TEXT(name, st, ...)                                             \
	{ _FIELD_TEXT(name, st, ##__VA_ARGS__) }

/** @brief Integer record field descriptor. */
#define FIELD_INT(name, st, ...)                                              \
	{ _FIELD_INT(name, st, ##__VA_ARGS__) }

/** @brief Boolean record field descriptor. */
#define FIELD_BOOL(name, st, ...)                                             \
	{ _FIELD_BOOL(name, st, ##__VA_ARGS__) }

/** @brief Reference descriptor with automatic source lookup and display. */
#define FIELD_REF(name, st, target, ...)                                      \
	{ _FIELD_REF(name, st, target, ##__VA_ARGS__) }

/** @brief Attached file descriptor (e.g. data.txt, pt_PT.html). */
#define FIELD_FILE(name, file_name, ...)                                      \
	{                                                                      \
		.key = #name,                                                  \
		.offset = 0,                                                   \
		.size = 0,                                                     \
		.is_int = 0,                                                   \
		.kind = HYLE_KIND_EXCLUDE,                                     \
		.qm_type = HYLE_CM_VSTR,                                       \
		.type = HYLE_FIELD_STRING,                                      \
		.writable = 1,                                                 \
		.file = file_name,                                             \
		##__VA_ARGS__                                                  \
	}

/** @brief Derived virtual field (in-memory derivation / search fold). */
#define FIELD_DERIVED(name, derive_func_key, ...)                             \
	{                                                                      \
		.key = #name,                                                  \
		.offset = 0,                                                   \
		.size = 0,                                                     \
		.is_int = 0,                                                   \
		.kind = HYLE_KIND_EXCLUDE,                                     \
		.qm_type = HYLE_CM_STR,                                        \
		.type = HYLE_FIELD_DERIVED,                                     \
		.writable = 0,                                                 \
		.derive_key = derive_func_key,                                 \
		##__VA_ARGS__                                                  \
	}

/** @brief Inverse virtual relation descriptor. */
#define FIELD_INVERSE(name, target, inv_field, ...)                            \
	{                                                                      \
		.key = #name,                                                  \
		.offset = 0,                                                   \
		.size = 0,                                                     \
		.is_int = 0,                                                   \
		.kind = HYLE_KIND_INVERSE,                                     \
		.qm_type = 0,                                                  \
		.type = HYLE_FIELD_INVERSE,                                     \
		.writable = 0,                                                 \
		.ref_source = target,                                          \
		.ref_inverse = inv_field,                                      \
		##__VA_ARGS__                                                  \
	}

/** @brief Excluded field backed by a struct member (e.g. owner). */
#define FIELD_EXCL(name, st, ...)                                             \
	{                                                                      \
		.key = #name,                                                  \
		.offset = offsetof(st, name),                                  \
		.size = sizeof(((st *)0)->name),                               \
		.is_int = 0,                                                   \
		.kind = HYLE_KIND_EXCLUDE,                                     \
		.qm_type = HYLE_CM_STR,                                        \
		.type = HYLE_FIELD_STRING,                                      \
		##__VA_ARGS__                                                  \
	}

/** @brief Integer overlay field for app state structures. */
#define OVERLAY_INT(name, st, mb)                                              \
	{                                                                      \
		.key = #name,                                                  \
		.offset = offsetof(st, mb),                                    \
		.size = sizeof(int),                                           \
		.is_int = 1,                                                   \
		.kind = HYLE_KIND_OVERLAY_INT                                  \
	}

/** @brief String overlay field for app state structures. */
#define OVERLAY_STR(name, st, mb, sz)                                          \
	{                                                                      \
		.key = #name,                                                  \
		.offset = offsetof(st, mb),                                    \
		.size = sz,                                                    \
		.is_int = 0,                                                   \
		.kind = HYLE_KIND_OVERLAY_STR                                  \
	}

/** @brief End marker for descriptor arrays (a NULL-key entry). */
#define FIELD_END { .key = NULL }

/* ── Higher-Order Array Combinator ─────────────────────────────── */
/** @brief Wraps any base field macro into an array/collection field. */
#define FIELD_ARRAY(type_macro, ...) type_macro(__VA_ARGS__, .is_array = 1)

/* Direct Array Convenience Aliases */
/** @brief Array-of-integers convenience alias. */
#define FIELD_ARRAY_INT(name, st, ...)                                        \
	FIELD_INT(name, st, .is_array = 1, ##__VA_ARGS__)
/** @brief Array-of-text convenience alias. */
#define FIELD_ARRAY_TEXT(name, st, ...)                                       \
	FIELD_TEXT(name, st, .is_array = 1, ##__VA_ARGS__)
/** @brief Array-of-bool convenience alias. */
#define FIELD_ARRAY_BOOL(name, st, ...)                                       \
	FIELD_BOOL(name, st, .is_array = 1, ##__VA_ARGS__)
/** @brief Array-of-references convenience alias. */
#define FIELD_ARRAY_REF(name, st, target, ...)                                \
	FIELD_REF(name, st, target, .is_array = 1, ##__VA_ARGS__)
/** @brief Alias of FIELD_ARRAY_REF for multi-reference fields. */
#define FIELD_MULTI_REF FIELD_ARRAY_REF

/* ── Legacy Positional Field Macros (for backward compatibility) ── */
/** @brief Legacy positional record (string) field. */
#define REC_FIELD(name, st, mb, sz, wr, rq, ml, im)                            \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_RECORD,             \
		HYLE_CM_STR, { .type = HYLE_FIELD_STRING }, wr, rq, ml,        \
		NULL, NULL, im, NULL, NULL, NULL, NULL, 0                      \
	}

/** @brief Legacy positional reference field. */
#define REF_FIELD(name, st, mb, sz, src, inv, im)                              \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_REF_DISPLAY,         \
		HYLE_CM_REFERENCE, { .type = HYLE_FIELD_REFERENCE }, 1, 0, 0,  \
		src, inv, im, #name, NULL, NULL, NULL, 0                       \
	}

/** @brief Legacy positional reference field with filter style. */
#define REF_FIELD_S(name, st, mb, sz, src, inv, im, style)                     \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_REF_DISPLAY,         \
		HYLE_CM_REFERENCE, { .type = HYLE_FIELD_REFERENCE }, 1, 0, 0,  \
		src, inv, im, #name, style, NULL, NULL, 0                      \
	}

/** @brief Legacy positional reference field with style and add flag. */
#define REF_FIELD_SA(name, st, mb, sz, src, inv, im, style, add)              \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_REF_DISPLAY,         \
		HYLE_CM_REFERENCE, { .type = HYLE_FIELD_REFERENCE }, 1, 0, 0,  \
		src, inv, im, #name, style, NULL, NULL, add                    \
	}

/** @brief Legacy positional multi-reference field. */
#define MULTI_REF_FIELD(name, st, mb, sz, src, inv, im)                        \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_REF_DISPLAY,         \
		HYLE_CM_MULTI_REF, { .type = HYLE_FIELD_MULTI_REFERENCE },     \
		1, 0, 0, src, inv, im, #name, NULL, NULL, NULL, 0              \
	}

/** @brief Legacy positional multi-reference field with style. */
#define MULTI_REF_FIELD_S(name, st, mb, sz, src, inv, im, style)               \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_REF_DISPLAY,         \
		HYLE_CM_MULTI_REF, { .type = HYLE_FIELD_MULTI_REFERENCE },     \
		1, 0, 0, src, inv, im, #name, style, NULL, NULL, 0             \
	}

/** @brief Legacy positional multi-reference field with style and mode. */
#define MULTI_REF_FIELD_SM(name, st, mb, sz, src, inv, im, style, mode)        \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_REF_DISPLAY,         \
		HYLE_CM_MULTI_REF, { .type = HYLE_FIELD_MULTI_REFERENCE },     \
		1, 0, 0, src, inv, im, #name, style, mode, NULL, 0             \
	}

/** @brief Legacy positional multi-reference field with style, mode and add. */
#define MULTI_REF_FIELD_SMA(name, st, mb, sz, src, inv, im, style, mode, add)  \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_REF_DISPLAY,         \
		HYLE_CM_MULTI_REF, { .type = HYLE_FIELD_MULTI_REFERENCE },     \
		1, 0, 0, src, inv, im, #name, style, mode, NULL, add          \
	}

/** @brief Legacy positional inverse field. */
#define INVERSE_FIELD(name, src, inv)                                          \
	{                                                                      \
		#name, 0, 0, 0, HYLE_KIND_INVERSE, 0,                          \
		{ .type = HYLE_FIELD_INVERSE }, 0, 0, 0,                       \
		src, inv, 0, NULL, NULL, NULL, NULL, 0                         \
	}

/** @brief Legacy positional excluded (string) field. */
#define EXCL_FIELD(name, st, mb, sz, ...)                                      \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_EXCLUDE,             \
		HYLE_CM_STR, { .type = HYLE_FIELD_STRING }, 0, 0, 0,            \
		NULL, NULL, 0, NULL, NULL, NULL, NULL, 0                       \
	}

/** @brief Legacy positional excluded field with meta flag. */
#define EXCL_FIELD_M(name, st, mb, sz, im)                                     \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_EXCLUDE,             \
		HYLE_CM_STR, { .type = HYLE_FIELD_STRING }, 0, 0, 0,            \
		NULL, NULL, im, NULL, NULL, NULL, NULL, 0                      \
	}

/** @brief Legacy positional excluded field with corm type and meta flag. */
#define EXCL_FIELD_W(name, st, mb, sz, qt, im)                                 \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_EXCLUDE, qt,          \
		{ .type = HYLE_FIELD_STRING }, 1, 0, 0, NULL, NULL, im,        \
		NULL, NULL, NULL, NULL, 0                                      \
	}

/** @brief Legacy virtual excluded (string) field. */
#define EXCL_FIELD_V(name, qt, wr, im)                                         \
	{                                                                      \
		#name, 0, 0, 0, HYLE_KIND_EXCLUDE, qt,                         \
		{ .type = HYLE_FIELD_STRING }, wr, 0, 0,                        \
		NULL, NULL, im, NULL, NULL, NULL, NULL, 0                      \
	}

/** @brief Legacy virtual excluded field with file name. */
#define EXCL_FIELD_VF(name, qt, wr, im, fl)                                    \
	{                                                                      \
		#name, 0, 0, 0, HYLE_KIND_EXCLUDE, qt,                         \
		{ .type = HYLE_FIELD_STRING }, wr, 0, 0,                        \
		NULL, NULL, im, fl, NULL, NULL, NULL, 0                        \
	}

/** @brief Legacy positional derived field. */
#define DERIVED_FIELD(name, dkey)                                              \
	{                                                                      \
		#name, 0, 0, 0, HYLE_KIND_EXCLUDE, HYLE_CM_STR,                \
		{ .type = HYLE_FIELD_DERIVED }, 0, 0, 0, NULL, NULL, 0, NULL,  \
		NULL, NULL, dkey, 0                                            \
	}

/** @brief Legacy positional integer field. */
#define INT_FIELD(name, st, mb, wr)                                            \
	{                                                                      \
		#name, offsetof(st, mb), sizeof(int), 1, HYLE_KIND_RECORD,     \
		0, { .type = HYLE_FIELD_INT }, wr, 0, 0, NULL, NULL, 0,        \
		NULL, NULL, NULL, NULL, 0                                      \
	}

/** @brief Legacy positional boolean field. */
#define BOOL_FIELD(name, st, mb, wr)                                           \
	{                                                                      \
		#name, offsetof(st, mb), sizeof(int), 0, HYLE_KIND_RECORD,     \
		0, { .type = HYLE_FIELD_BOOL }, wr, 0, 0, NULL, NULL, 0,       \
		NULL, NULL, NULL, NULL, 0                                      \
	}

/** @brief Legacy positional required (string) field. */
#define REQ_FIELD(name, st, mb, sz)                                            \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_RECORD,              \
		HYLE_CM_STR, { .type = HYLE_FIELD_STRING }, 1, 1, 0,           \
		NULL, NULL, 0, NULL, NULL, NULL, NULL, 0                       \
	}

/** @brief Legacy positional required field with minimum length. */
#define REQ_FIELD_MIN(name, st, mb, sz, ml)                                    \
	{                                                                      \
		#name, offsetof(st, mb), sz, 0, HYLE_KIND_RECORD,              \
		HYLE_CM_STR, { .type = HYLE_FIELD_STRING }, 1, 1, ml,          \
		NULL, NULL, 0, NULL, NULL, NULL, NULL, 0                       \
	}

/** @brief Legacy variable-length string (file) field. */
#define VSTR_FIELD(name, fl)                                                   \
	{                                                                      \
		#name, 0, 0, 0, HYLE_KIND_EXCLUDE, HYLE_CM_VSTR,               \
		{ .type = HYLE_FIELD_STRING }, 1, 0, 0, NULL, NULL, 0,         \
		fl, NULL, NULL, NULL, 0                                        \
	}

/** @brief Alias of VSTR_FIELD for attached files. */
#define FILE_FIELD(name, file_name) VSTR_FIELD(name, file_name)

#endif
