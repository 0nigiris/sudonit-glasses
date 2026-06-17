#include "sudonit/protocol/json.h"

#include <stdio.h>
#include <string.h>

/* Append one Unicode code point to out[] as UTF-8, respecting capacity. */
static void append_utf8(char *out, size_t cap, size_t *pos, unsigned cp) {
    if (cp < 0x80) {
        if (*pos + 1 < cap) out[(*pos)++] = (char)cp;
    } else if (cp < 0x800) {
        if (*pos + 2 < cap) {
            out[(*pos)++] = (char)(0xC0 | (cp >> 6));
            out[(*pos)++] = (char)(0x80 | (cp & 0x3F));
        }
    } else if (cp < 0x10000) {
        if (*pos + 3 < cap) {
            out[(*pos)++] = (char)(0xE0 | (cp >> 12));
            out[(*pos)++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[(*pos)++] = (char)(0x80 | (cp & 0x3F));
        }
    } else {
        if (*pos + 4 < cap) {
            out[(*pos)++] = (char)(0xF0 | (cp >> 18));
            out[(*pos)++] = (char)(0x80 | ((cp >> 12) & 0x3F));
            out[(*pos)++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[(*pos)++] = (char)(0x80 | (cp & 0x3F));
        }
    }
}

static int hex4(const char *p, unsigned *out) {
    unsigned v = 0;
    for (int i = 0; i < 4; ++i) {
        char c = p[i];
        v <<= 4;
        if (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
        else return -1;
    }
    *out = v;
    return 0;
}

sd_err_t sd_json_get_string(const char *json, const char *key, char *out, size_t cap) {
    if (!json || !key || !out || cap == 0) {
        return SD_ERR_INVALID;
    }

    /* Locate "key" then the following ':' then the opening '"'. */
    char needle[64];
    int n = snprintf(needle, sizeof(needle), "\"%s\"", key);
    if (n < 0 || (size_t)n >= sizeof(needle)) {
        return SD_ERR_INVALID;
    }
    const char *p = strstr(json, needle);
    if (!p) {
        return SD_ERR_INVALID;
    }
    p += n;
    while (*p == ' ' || *p == '\t') ++p;
    if (*p != ':') return SD_ERR_INVALID;
    ++p;
    while (*p == ' ' || *p == '\t') ++p;
    if (*p != '"') return SD_ERR_INVALID;
    ++p;

    size_t pos = 0;
    while (*p && *p != '"') {
        if (*p == '\\') {
            ++p;
            switch (*p) {
                case '"': append_utf8(out, cap, &pos, '"'); break;
                case '\\': append_utf8(out, cap, &pos, '\\'); break;
                case '/': append_utf8(out, cap, &pos, '/'); break;
                case 'b': append_utf8(out, cap, &pos, '\b'); break;
                case 'f': append_utf8(out, cap, &pos, '\f'); break;
                case 'n': append_utf8(out, cap, &pos, '\n'); break;
                case 'r': append_utf8(out, cap, &pos, '\r'); break;
                case 't': append_utf8(out, cap, &pos, '\t'); break;
                case 'u': {
                    unsigned cp;
                    if (hex4(p + 1, &cp) != 0) return SD_ERR_INVALID;
                    p += 4;
                    /* High surrogate followed by a low surrogate -> combine. */
                    if (cp >= 0xD800 && cp <= 0xDBFF && p[1] == '\\' && p[2] == 'u') {
                        unsigned lo;
                        if (hex4(p + 3, &lo) == 0 && lo >= 0xDC00 && lo <= 0xDFFF) {
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                            p += 6;
                        }
                    }
                    append_utf8(out, cap, &pos, cp);
                    break;
                }
                default:
                    return SD_ERR_INVALID;
            }
            if (*p) ++p;
        } else {
            if (pos + 1 < cap) out[pos++] = *p;
            ++p;
        }
    }
    out[pos < cap ? pos : cap - 1] = '\0';
    return SD_OK;
}
