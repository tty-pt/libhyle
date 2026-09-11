#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <bud/bud_app.h>
#include <hyle-bud/hyle-bud.h>

/* ── Shared multi-value selection helpers ─────────────────────── */

static int hyle_bud_comma_split(const char *cur, const char **out, int max)
{
	int n = 0;

	if (!cur || !cur[0])
		return 0;
	while (*cur && n < max) {
		const char *comma = strchr(cur, ',');
		size_t len = comma ? (size_t)(comma - cur) : strlen(cur);
		if (len > 0) {
			out[n] = cur;
			n++;
		}
		if (!comma)
			break;
		cur = comma + 1;
	}
	return n;
}

static int hyle_bud_is_selected(
	const char *const *sel, int nsel, const char *id)
{
	size_t olen;
	int j;

	if (!id)
		return 0;
	olen = strlen(id);
	for (j = 0; j < nsel; j++) {
		if (strncmp(sel[j], id, olen) == 0 &&
		    (sel[j][olen] == '\0' || sel[j][olen] == ','))
			return 1;
	}
	return 0;
}

static bud_node *hyle_bud_boolean_checkbox(
	const char *key,
	const char *label,
	const char *current_value)
{
	int is_checked = current_value && (
		strcmp(current_value, "true") == 0 ||
		strcmp(current_value, "1") == 0 ||
		strcmp(current_value, "on") == 0
	);
	return bud_tpl(
		"<fieldset>"
		"  <legend></legend>"
		"  <label>"
		"    <input type='checkbox' name='%s' value='1' %b/> %s"
		"  </label>"
		"</fieldset>",
		key ? key : "",
		is_checked ? "checked" : NULL,
		label ? label : ""
	);
}

static bud_node *hyle_bud_checkbox_fieldset(
	const char *key,
	const char *label,
	const char *current_value,
	const hyle_bud_option_t *options,
	int noptions)
{
	const char *selected[1024];
	int nselected;
	bud_node *fs;
	int i;

	nselected = hyle_bud_comma_split(current_value, selected, 1024);

	fs = bud_tpl(
		"<fieldset class='hyle-checkbox-filter'>"
		"  <legend>%s</legend>"
		"</fieldset>",
		label ? label : ""
	);

	for (i = 0; i < noptions; i++) {
		int checked =
		        hyle_bud_is_selected(selected, nselected, options[i].id);
		bud_node *lbl = bud_tpl(
			"<label>"
			"  <input type='checkbox' name='%s' value='%s' %b/> %s"
			"</label>",
			key ? key : "",
			options[i].id ? options[i].id : "",
			checked ? "checked" : NULL,
			options[i].label ? options[i].label : ""
		);
		if (lbl)
			bud_append(fs, lbl);
	}

	return fs;
}

/* ── Multi-select dropdown widget (SSR-first, WASM-enhanced) ──── */

void hyle_bud_ms_reset(void)
{
}

static bud_node *find_closest_with_attr(bud_node *node, const char *attr_name)
{
	bud_node *curr = node;
	while (curr) {
		if (bud_get_attr(curr, attr_name))
			return curr;
		curr = bud_node_parent(curr);
	}
	return NULL;
}

static bud_node *find_descendant_with_attr(bud_node *node, const char *attr_name)
{
	if (!node)
		return NULL;
	if (bud_get_attr(node, attr_name))
		return node;
	for (size_t i = 0; i < bud_node_child_count(node); i++) {
		bud_node *found = find_descendant_with_attr(
		        (bud_node *)bud_node_child(node, i), attr_name);
		if (found)
			return found;
	}
	return NULL;
}

static const char *ms_ci_substr(const char *haystack, const char *needle)
{
	size_t i;

	if (!needle || !needle[0])
		return haystack;
	if (!haystack)
		return NULL;
	for (; *haystack; haystack++) {
		for (i = 0; needle[i] && haystack[i]; i++) {
			char a = haystack[i];
			char b = needle[i];
			if (a >= 'A' && a <= 'Z')
				a += 32;
			if (b >= 'A' && b <= 'Z')
				b += 32;
			if (a != b)
				break;
		}
		if (!needle[i])
			return haystack;
		if (!haystack[i])
			break;
	}
	return NULL;
}

static int hyle_bud_ms_on_search(bud_event *event)
{
	bud_node *details;
	bud_node *opts_container;
	const char *needle;
	size_t count;
	size_t i;

	if (!event)
		return 0;
	details = find_closest_with_attr(event->target, "data-hyle-ms");
	if (!details)
		return 0;
	opts_container = find_descendant_with_attr(details, "data-hyle-ms-options");
	if (!opts_container)
		return 0;

	needle = (const char *)event->user;
	if (!needle)
		needle = "";

	count = bud_node_child_count(opts_container);
	for (i = 0; i < count; i++) {
		bud_node *row = (bud_node *)bud_node_child(opts_container, i);
		const char *lbl = bud_get_attr(row, "data-label");
		int visible = ms_ci_substr(lbl ? lbl : "", needle) != NULL;
		bud_patch_attr(row, "class",
		               visible ? "hyle-ms-option"
		                       : "hyle-ms-option hyle-ms-hidden");
	}
	return 0;
}

static int hyle_bud_ms_on_change(bud_event *event)
{
	bud_node *details;
	bud_node *opts_container;
	bud_node *values_span;
	bud_node *text_node;
	const char *label;
	int now;
	size_t count;
	size_t pos = 0;
	int shown = 0;
	char summary[4096];

	if (!event)
		return 0;
	details = find_closest_with_attr(event->target, "data-hyle-ms");
	if (!details)
		return 0;
	opts_container = find_descendant_with_attr(details, "data-hyle-ms-options");
	values_span = find_descendant_with_attr(details, "data-hyle-ms-values");
	if (!opts_container || !values_span)
		return 0;

	now = event->user && ((const char *)event->user)[0] == '1';
	bud_set_attr(event->target, "data-checked", now ? "1" : "0");

	summary[0] = '\0';
	count = bud_node_child_count(opts_container);
	for (size_t i = 0; i < count; i++) {
		bud_node *row = (bud_node *)bud_node_child(opts_container, i);
		bud_node *cb = bud_node_child_count(row) > 0
		                      ? (bud_node *)bud_node_child(row, 0)
		                      : NULL;
		const char *chk = cb ? bud_get_attr(cb, "data-checked") : NULL;
		int is_chk = 0;
		if (chk) {
			is_chk = (chk[0] == '1');
		} else if (cb && bud_get_attr(cb, "checked")) {
			is_chk = 1;
		}
		if (!is_chk)
			continue;

		const char *lbl = bud_get_attr(row, "data-label");
		if (!lbl)
			lbl = "";
		if (pos + 1 >= sizeof(summary))
			break;
		int n = snprintf(summary + pos, sizeof(summary) - pos, "%s%s",
		                 shown ? "; " : "", lbl);
		if (n < 0 || (size_t)n >= sizeof(summary) - pos)
			break;
		pos += (size_t)n;
		shown = 1;
	}

	if (!shown) {
		label = bud_get_attr(details, "data-hyle-ms-label");
		snprintf(summary, sizeof(summary), "All %ss", label ? label : "");
	}

	text_node = bud_node_child_count(values_span) > 0
	                    ? (bud_node *)bud_node_child(values_span, 0)
	                    : values_span;
	bud_patch_text(text_node, summary);
	return 0;
}

bud_node *hyle_bud_multiselect_field(
	const char *key,
	const char *label,
	const char *current_value,
	const hyle_bud_option_t *options,
	int noptions)
{
	const char *selected[1024];
	int nselected;
	bud_node *summary;
	bud_node *summary_text_node;
	bud_node *search;
	bud_node *container;
	bud_node *caption;
	char summary_text[4096];
	size_t pos = 0;
	int shown = 0;
	int i;

	if (!options || noptions <= 0)
		return NULL;

	nselected = hyle_bud_comma_split(current_value, selected, 1024);

	summary_text[0] = '\0';
	for (i = 0; i < noptions; i++) {
		int is_sel = hyle_bud_is_selected(selected, nselected, options[i].id);
		if (!is_sel)
			continue;
		if (pos + 1 >= sizeof(summary_text))
			break;
		int n = snprintf(summary_text + pos, sizeof(summary_text) - pos, "%s%s",
		                 shown ? "; " : "", options[i].label ? options[i].label : "");
		if (n < 0 || (size_t)n >= sizeof(summary_text) - pos)
			break;
		pos += (size_t)n;
		shown = 1;
	}
	if (!shown)
		snprintf(summary_text, sizeof(summary_text), "All %ss", label ? label : "");

	summary_text_node = bud_text(summary_text);
	summary = bud_tpl(
		"<span class='hyle-ms-values' data-hyle-ms-values='1'>%node</span>",
		summary_text_node
	);

	caption = bud_tpl("<span class='hyle-ms-caption'>%s</span>", label ? label : "");

	container = bud_tpl("<div class='hyle-ms-options' data-hyle-ms-options='1'></div>");

	for (i = 0; i < noptions; i++) {
		int is_sel = hyle_bud_is_selected(selected, nselected, options[i].id);
		bud_node *cb = bud_tpl(
			"<input type='checkbox' name='%s' value='%s' %b data-checked='%s' %bind/>",
			key ? key : "",
			options[i].id ? options[i].id : "",
			is_sel ? "checked" : NULL,
			is_sel ? "1" : "0",
			"change", hyle_bud_ms_on_change
		);
		bud_node *row = bud_tpl(
			"<label class='hyle-ms-option' data-label='%s'>"
			"  %node %s"
			"</label>",
			options[i].label ? options[i].label : "",
			cb,
			options[i].label ? options[i].label : ""
		);
		bud_append(container, row);
	}

	search = bud_tpl(
		"<input type='search' class='hyle-ms-search' data-hyle-ms-search='1' placeholder='%s' aria-label='%s' %bind/>",
		hyle_bud_tr("Search…"), hyle_bud_tr("Search options"),
		"input", hyle_bud_ms_on_search
	);

	return bud_tpl(
		"<div class='hyle-ms-field'>"
		"  %node"
		"  <details class='hyle-multiselect' data-hyle-ms='%s' data-hyle-ms-label='%s'>"
		"    <summary class='hyle-ms-trigger'>"
		"      %node"
		"      <span class='hyle-ms-caret' aria-hidden='true'>▾</span>"
		"    </summary>"
		"    <div class='hyle-ms-panel' data-hyle-ms-panel='1'>"
		"      %node"
		"      %node"
		"    </div>"
		"  </details>"
		"</div>",
		caption,
		key ? key : "",
		label ? label : "",
		summary,
		search,
		container
	);
}

static bud_node *hyle_bud_reference_select(
	const char *key,
	const char *label,
	const char *current_value,
	const hyle_bud_option_t *options,
	int noptions)
{
	char all_label[256];
	int i;

	snprintf(all_label, sizeof(all_label), "All %ss", label ? label : "");

	bud_node *select = bud_tpl(
		"<select name='%s'>"
		"  <option value='' %b>%s</option>"
		"</select>",
		key ? key : "",
		(!current_value || !current_value[0]) ? "selected" : NULL,
		all_label
	);

	for (i = 0; i < noptions; i++) {
		int sel = current_value && strcmp(current_value, options[i].id) == 0;
		bud_node *opt = bud_tpl(
			"<option value='%s' %b>%s</option>",
			options[i].id ? options[i].id : "",
			sel ? "selected" : NULL,
			options[i].label ? options[i].label : ""
		);
		if (opt)
			bud_append(select, opt);
	}

	return bud_tpl(
		"<label>%s %node</label>",
		label ? label : "",
		select
	);
}

/* ── Dropdown single-select widget (SSR-first, WASM-enhanced) ─── */

static int hyle_bud_ss_on_search(bud_event *event)
{
	bud_node *details;
	bud_node *opts_container;
	const char *needle;
	size_t count;
	size_t i;

	if (!event)
		return 0;
	details = find_closest_with_attr(event->target, "data-hyle-ss");
	if (!details)
		return 0;
	opts_container = find_descendant_with_attr(details, "data-hyle-ss-options");
	if (!opts_container)
		return 0;

	needle = (const char *)event->user;
	if (!needle)
		needle = "";

	count = bud_node_child_count(opts_container);
	for (i = 0; i < count; i++) {
		bud_node *row = (bud_node *)bud_node_child(opts_container, i);
		const char *lbl = bud_get_attr(row, "data-label");
		int visible = ms_ci_substr(lbl ? lbl : "", needle) != NULL;
		bud_patch_attr(row, "class",
		               visible ? "hyle-ss-option"
		                       : "hyle-ss-option hyle-ss-hidden");
	}
	return 0;
}

static int hyle_bud_ss_on_change(bud_event *event)
{
	bud_node *details;
	bud_node *values_span;
	bud_node *text_node;
	bud_node *row;
	const char *lbl;

	if (!event)
		return 0;
	details = find_closest_with_attr(event->target, "data-hyle-ss");
	if (!details)
		return 0;
	values_span = find_descendant_with_attr(details, "data-hyle-ss-values");
	if (!values_span)
		return 0;

	row = bud_node_parent(event->target);
	lbl = row ? bud_get_attr(row, "data-label") : NULL;
	if (!lbl)
		lbl = bud_get_attr(event->target, "value");

	text_node = bud_node_child_count(values_span) > 0
	                    ? (bud_node *)bud_node_child(values_span, 0)
	                    : values_span;
	bud_patch_text(text_node, lbl ? lbl : "");
	return 0;
}

bud_node *hyle_bud_reference_select_dropdown(
	const char *key,
	const char *label,
	const char *current_value,
	const hyle_bud_option_t *options,
	int noptions)
{
	bud_node *summary_text_node;
	bud_node *summary;
	bud_node *search;
	bud_node *container;
	bud_node *caption;
	const char *current_label = NULL;
	char summary_text[4096];
	int i;

	if (!options || noptions <= 0)
		return NULL;

	for (i = 0; i < noptions; i++) {
		if (current_value &&
		    strcmp(current_value, options[i].id) == 0)
			current_label = options[i].label;
	}

	if (current_label)
		snprintf(summary_text, sizeof(summary_text), "%s",
		         current_label);
	else
		snprintf(summary_text, sizeof(summary_text), "All %ss", label ? label : "");

	summary_text_node = bud_text(summary_text);
	summary = bud_tpl(
		"<span class='hyle-ss-values' data-hyle-ss-values='1'>%node</span>",
		summary_text_node
	);

	caption = bud_tpl("<span class='hyle-ss-caption'>%s</span>", label ? label : "");

	container = bud_tpl("<div class='hyle-ss-options' data-hyle-ss-options='1'></div>");

	for (i = 0; i < noptions; i++) {
		int sel =
		        current_value &&
		        strcmp(current_value, options[i].id) == 0;
		bud_node *radio = bud_tpl(
			"<input type='radio' name='%s' value='%s' %b %bind/>",
			key ? key : "",
			options[i].id ? options[i].id : "",
			sel ? "checked" : NULL,
			"change", hyle_bud_ss_on_change
		);
		bud_node *row = bud_tpl(
			"<label class='hyle-ss-option' data-label='%s'>"
			"  %node %s"
			"</label>",
			options[i].label ? options[i].label : "",
			radio,
			options[i].label ? options[i].label : ""
		);
		bud_append(container, row);
	}

	search = bud_tpl(
		"<input type='search' class='hyle-ss-search' data-hyle-ss-search='1' placeholder='%s' aria-label='%s' %bind/>",
		hyle_bud_tr("Search…"), hyle_bud_tr("Search options"),
		"input", hyle_bud_ss_on_search
	);

	return bud_tpl(
		"<div class='hyle-ss-field'>"
		"  %node"
		"  <details class='hyle-singleselect' data-hyle-ss='%s' data-hyle-ss-label='%s'>"
		"    <summary class='hyle-ss-trigger'>"
		"      %node"
		"      <span class='hyle-ss-caret' aria-hidden='true'>▾</span>"
		"    </summary>"
		"    <div class='hyle-ss-panel' data-hyle-ss-panel='1'>"
		"      %node"
		"      %node"
		"    </div>"
		"  </details>"
		"</div>",
		caption,
		key ? key : "",
		label ? label : "",
		summary,
		search,
		container
	);
}

bud_node *hyle_bud_text_input(
	const char *key,
	const char *label,
	const char *current_value)
{
	return bud_tpl(
		"<label class='filter-field'>%s: "
		"  <input type='text' name='%s' placeholder='%s' value='%s'/>"
		"</label>",
		label ? label : "",
		key ? key : "",
		label ? label : "",
		(current_value && current_value[0]) ? current_value : ""
	);
}

bud_node *hyle_bud_filter_field(
	const char *key,
	const char *label,
	int type,
	const char *current_value,
	const hyle_bud_option_t *options,
	int noptions,
	const char *filter_style)
{
	switch (type) {
	case HYLE_FIELD_BOOL:
		return hyle_bud_boolean_checkbox(key, label, current_value);
	case HYLE_FIELD_MULTI_REFERENCE:
		if (options && noptions > 0) {
			if (filter_style && strcmp(filter_style, "dropdown") == 0)
				return hyle_bud_multiselect_field(
				        key, label, current_value, options,
				        noptions);
			return hyle_bud_checkbox_fieldset(
			        key, label, current_value, options, noptions);
		}
		return hyle_bud_text_input(key, label, current_value);
	case HYLE_FIELD_REFERENCE:
		if (options && noptions > 0) {
			if (filter_style &&
			    strcmp(filter_style, "dropdown") == 0)
				return hyle_bud_reference_select_dropdown(
				        key, label, current_value, options,
				        noptions);
			if (filter_style &&
			    strcmp(filter_style, "multiselect") == 0)
				return hyle_bud_multiselect_field(
				        key, label, current_value, options,
				        noptions);
			if (filter_style && strcmp(filter_style, "grid") == 0)
				return hyle_bud_checkbox_fieldset(
				        key, label, current_value, options,
				        noptions);
			/* "select" (explicit) or absent -> native <select> */
			return hyle_bud_reference_select(
			        key, label, current_value, options, noptions);
		}
		return hyle_bud_text_input(key, label, current_value);
	default:
		return hyle_bud_text_input(key, label, current_value);
	}
}

bud_node *hyle_bud_filter_from_schema(
	const hyle_schema_desc_t *desc,
	const char *field_name,
	const char *current_value,
	const hyle_bud_option_t *options,
	int noptions)
{
	const hyle_schema_desc_t *d;
	char auto_label[128];
	const char *label = field_name;

	if (!desc || !field_name || !field_name[0])
		return NULL;

	/* Look up field in schema */
	for (d = desc; d && d->key; d++) {
		if (strcmp(d->key, field_name) == 0)
			break;
	}

	if (!d || !d->key) {
		/* Not explicitly found in schema, default to text search */
		return hyle_bud_text_input(field_name, field_name, current_value);
	}

	/* Derive display label */
	if (field_name && field_name[0]) {
		size_t bi = 0;
		int cap_next = 1;
		for (size_t i = 0; field_name[i] && bi + 1 < sizeof(auto_label); i++) {
			char c = field_name[i];
			if (c == '_' || c == '-') {
				if (bi > 0 && auto_label[bi - 1] != ' ')
					auto_label[bi++] = ' ';
				cap_next = 1;
			} else if (cap_next && c >= 'a' && c <= 'z') {
				auto_label[bi++] = c - 32;
				cap_next = 0;
			} else {
				auto_label[bi++] = c;
				cap_next = 0;
			}
		}
		auto_label[bi] = '\0';
		label = auto_label;
	}

	return hyle_bud_filter_field(
	        d->key, label, d->source_type, current_value, options, noptions,
	        d->filter_style ? d->filter_style : "dropdown");
}

bud_node *hyle_bud_filter(
	const hyle_schema_desc_t *desc,
	const char *field_name,
	const char *current_value,
	const hyle_bud_picker_view_t *pv)
{
	return hyle_bud_filter_scoped(
	        desc, field_name, -1, current_value, current_value, NULL, pv,
	        0, NULL, NULL);
}

bud_node *hyle_bud_filter_group(
	const hyle_schema_desc_t *desc,
	const char **field_names,
	int n_fields,
	const char *current_qs,
	const hyle_bud_picker_view_t *pv)
{
	bud_node *frag = bud_fragment();
	char val_buf[512];
	int i;

	if (!desc || !field_names || n_fields <= 0)
		return frag;

	for (i = 0; i < n_fields; i++) {
		const char *fname = field_names[i];
		if (!fname || !fname[0])
			continue;

		val_buf[0] = '\0';
		if (current_qs) {
			hyle_bud_query_param(current_qs, fname, val_buf, sizeof(val_buf));
		}

		bud_node *node = hyle_bud_filter(desc, fname, val_buf, pv);
		if (node)
			bud_append(frag, node);
	}

	return frag;
}
