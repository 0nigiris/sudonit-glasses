/* Sudonit protocol — control messages and image transfer (C port).
 *
 * Mirrors protocol/messages.py: image_begin -> binary chunks -> image_end, with
 * a SHA-256 the phone verifies. Outgoing JSON is built by hand over controlled
 * values; incoming control messages are classified by `type`.
 */
#ifndef SUDONIT_PROTOCOL_MESSAGES_H
#define SUDONIT_PROTOCOL_MESSAGES_H

#include <stddef.h>
#include <stdint.h>

#include "sudonit/error.h"
#include "sudonit/hal/transport.h"

/* Chunk size for image transfer (matches the Python DEFAULT_CHUNK_SIZE). */
#define SD_CHUNK_SIZE 4096

typedef enum {
    SD_MSG_UNKNOWN = 0,
    SD_MSG_PONG,
    SD_MSG_DEVICE_INFO,
    SD_MSG_AI_RESPONSE,
    SD_MSG_PLAY_AUDIO,
    SD_MSG_ERROR
} sd_msg_type_t;

/* Send a {"type":"ping"} control frame. */
sd_err_t sd_msg_send_ping(sd_transport_t *t);

/* Send an image: image_begin (with sha256) + chunks + image_end. */
sd_err_t sd_msg_send_image(sd_transport_t *t, const char *image_id,
                           const uint8_t *data, size_t len, const char *media_type);

/* Receive and classify one control frame.
 *   *type_out : the message type
 *   text_out  : the relevant string (ai_response.text / play_audio.content /
 *               error.message), or "" if none. NUL-terminated, truncated to cap.
 * Binary frames are not expected inbound on the device and yield SD_ERR_IO.
 */
sd_err_t sd_msg_recv(sd_transport_t *t, sd_msg_type_t *type_out, char *text_out,
                     size_t text_cap);

#endif /* SUDONIT_PROTOCOL_MESSAGES_H */
