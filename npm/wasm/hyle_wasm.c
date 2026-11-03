#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ttypt/iso.h>

typedef struct {
    char name[64];
    int required;
    size_t min_length;
    size_t max_length;
} wasm_hyle_field_t;

ISO_EXPORT(int, hyle_wasm_validate_field, int required, size_t min_len, size_t max_len, const char *val) {
    if (required && (!val || val[0] == '\0')) {
        return 1; // required error
    }
    if (val && val[0] != '\0') {
        size_t len = strlen(val);
        if (min_len > 0 && len < min_len) return 2; // minLength error
        if (max_len > 0 && len > max_len) return 3; // maxLength error
    }
    return 0; // valid
}
