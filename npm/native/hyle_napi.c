#include <node_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <hyle/purify.h>
#include <hyle/field.h>

static napi_value HylePurify(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, NULL, NULL);

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected (fieldsArray, valuesObject)");
        return NULL;
    }

    uint32_t field_count = 0;
    napi_get_array_length(env, args[0], &field_count);

    if (field_count == 0) {
        napi_value empty_arr;
        napi_create_array_with_length(env, 0, &empty_arr);
        return empty_arr;
    }

    hyle_field_t *fields = (hyle_field_t *)calloc(field_count, sizeof(hyle_field_t));
    const char **values = (const char **)calloc(field_count, sizeof(char *));

    for (uint32_t i = 0; i < field_count; i++) {
        napi_value field_obj;
        napi_get_element(env, args[0], i, &field_obj);

        napi_value name_val;
        napi_get_named_property(env, field_obj, "name", &name_val);
        size_t name_len = 0;
        napi_get_value_string_utf8(env, name_val, NULL, 0, &name_len);
        char *name_str = (char *)malloc(name_len + 1);
        napi_get_value_string_utf8(env, name_val, name_str, name_len + 1, &name_len);
        fields[i].name = name_str;

        bool has_req = false;
        napi_has_named_property(env, field_obj, "required", &has_req);
        if (has_req) {
            bool req = false;
            napi_value v;
            napi_get_named_property(env, field_obj, "required", &v);
            napi_get_value_bool(env, v, &req);
            fields[i].required = req ? 1 : 0;
        }

        bool has_min_len = false;
        napi_has_named_property(env, field_obj, "minLength", &has_min_len);
        if (has_min_len) {
            uint32_t ml = 0;
            napi_value v;
            napi_get_named_property(env, field_obj, "minLength", &v);
            napi_get_value_uint32(env, v, &ml);
            fields[i].min_length = ml;
        }

        bool has_pat = false;
        napi_has_named_property(env, field_obj, "pattern", &has_pat);
        if (has_pat) {
            napi_value v;
            napi_get_named_property(env, field_obj, "pattern", &v);
            size_t pat_len = 0;
            napi_get_value_string_utf8(env, v, NULL, 0, &pat_len);
            if (pat_len > 0) {
                char *pat_str = (char *)malloc(pat_len + 1);
                napi_get_value_string_utf8(env, v, pat_str, pat_len + 1, &pat_len);
                fields[i].pattern = pat_str;
            }
        }

        /* Extract corresponding value from valuesObject */
        bool has_val = false;
        napi_has_named_property(env, args[1], name_str, &has_val);
        if (has_val) {
            napi_value v;
            napi_get_named_property(env, args[1], name_str, &v);
            napi_valuetype vt;
            napi_typeof(env, v, &vt);
            if (vt == napi_string) {
                size_t v_len = 0;
                napi_get_value_string_utf8(env, v, NULL, 0, &v_len);
                char *val_str = (char *)malloc(v_len + 1);
                napi_get_value_string_utf8(env, v, val_str, v_len + 1, &v_len);
                values[i] = val_str;
            }
        }
    }

    hyle_purify_error_t *errors = NULL;
    size_t error_count = 0;

    hyle_purify_row(fields, field_count, values, &errors, &error_count);

    napi_value out_arr;
    napi_create_array_with_length(env, error_count, &out_arr);

    for (size_t i = 0; i < error_count; i++) {
        napi_value err_obj;
        napi_create_object(env, &err_obj);

        napi_value f_val, r_val, m_val;
        napi_create_string_utf8(env, errors[i].field ? errors[i].field : "", NAPI_AUTO_LENGTH, &f_val);
        napi_create_string_utf8(env, errors[i].rule ? errors[i].rule : "", NAPI_AUTO_LENGTH, &r_val);
        napi_create_string_utf8(env, errors[i].message ? errors[i].message : "", NAPI_AUTO_LENGTH, &m_val);

        napi_set_named_property(env, err_obj, "field", f_val);
        napi_set_named_property(env, err_obj, "rule", r_val);
        napi_set_named_property(env, err_obj, "message", m_val);

        napi_set_element(env, out_arr, (uint32_t)i, err_obj);
    }

    if (errors && error_count > 0) {
        hyle_purify_errors_free(errors, error_count);
    }

    /* Cleanup allocated strings */
    for (uint32_t i = 0; i < field_count; i++) {
        if (fields[i].name) free((void *)fields[i].name);
        if (fields[i].pattern) free((void *)fields[i].pattern);
        if (values[i]) free((void *)values[i]);
    }
    free(fields);
    free(values);

    return out_arr;
}

static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"purifyRow", NULL, HylePurify, NULL, NULL, NULL, napi_default, NULL}
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
