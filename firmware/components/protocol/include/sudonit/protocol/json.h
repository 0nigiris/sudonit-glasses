/* Minimal JSON reading for the protocol's flat control messages.
 *
 * We do NOT pull in a full JSON library: the control messages are small, flat,
 * string/number objects ({"type":"ai_response","text":"..."}). Outgoing JSON is
 * built by hand with snprintf over values we control; this header covers the
 * only read we need — pull one string value out by key, unescaped.
 *
 * Limitation (acceptable for v1 control messages): the key is matched as the
 * token "key" followed by ':'. It is not a general-purpose parser.
 */
#ifndef SUDONIT_PROTOCOL_JSON_H
#define SUDONIT_PROTOCOL_JSON_H

#include <stddef.h>

#include "sudonit/error.h"

/* Find "key": "value" in `json` and copy the unescaped value into `out`
 * (NUL-terminated, truncated to `cap`). Handles \" \\ \/ \b \f \n \r \t and
 * \uXXXX (incl. surrogate pairs) -> UTF-8. Returns SD_ERR_INVALID if not found.
 */
sd_err_t sd_json_get_string(const char *json, const char *key, char *out, size_t cap);

#endif /* SUDONIT_PROTOCOL_JSON_H */
