#include <stdlib.h>
#include <string.h>
#include <hyle/url.h>

static size_t hyle_qs_decode(const char *src, size_t src_len, char *out, size_t out_len)
{
	size_t r = 0, w = 0;
	while (r < src_len && w + 1 < out_len) {
		if (src[r] == '%' && r + 2 < src_len) {
			char hex[3] = { src[r + 1], src[r + 2], 0 };
			char *end;
			long val = strtol(hex, &end, 16);
			if (end == hex + 2 && val >= 0) {
				out[w++] = (char)val;
				r += 3;
				continue;
			}
		}
		if (src[r] == '+')
			out[w++] = ' ';
		else
			out[w++] = src[r];
		r++;
	}
	out[w] = '\0';
	return w;
}

size_t hyle_qs_param(const char *qs, const char *key, char *out, size_t out_sz)
{
	size_t klen;
	const char *p;

	if (out && out_sz)
		out[0] = '\0';
	if (!qs || !key || !out || out_sz == 0)
		return 0;
	klen = strlen(key);
	p = qs;
	while (p && *p) {
		const char *amp = strchr(p, '&');
		size_t part_len = amp ? (size_t)(amp - p) : strlen(p);
		const char *eq = memchr(p, '=', part_len);
		if (eq && (size_t)(eq - p) == klen && strncmp(p, key, klen) == 0) {
			const char *v = eq + 1;
			size_t vlen = part_len - klen - 1;
			return hyle_qs_decode(v, vlen, out, out_sz);
		}
		p = amp ? amp + 1 : NULL;
	}
	return 0;
}