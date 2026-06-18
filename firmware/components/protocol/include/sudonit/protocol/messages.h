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
    SD_MSG_AUDIO_BEGIN,
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
 *               For SD_MSG_AUDIO_BEGIN, text_out holds the raw begin JSON so the
 *               caller can read size/chunks/sample_rate/channels/sha256 from it.
 * Binary frames are not expected here and yield SD_ERR_IO — an audio downlink is
 * read with sd_msg_recv_audio_body() after an audio_begin is seen.
 */
sd_err_t sd_msg_recv(sd_transport_t *t, sd_msg_type_t *type_out, char *text_out,
                     size_t text_cap);

/* Receive the body of an audio downlink (the `chunks` binary frames + audio_end)
 * after an audio_begin was classified by sd_msg_recv. The reassembled PCM is
 * written into `pcm` (must hold at least `size` bytes) and verified against
 * `sha_hex`. Returns SD_OK only if the count, size, and SHA-256 all match. */
sd_err_t sd_msg_recv_audio_body(sd_transport_t *t, uint32_t chunks, uint32_t size,
                                const char *sha_hex, uint8_t *pcm, size_t pcm_cap);

#endif /* SUDONIT_PROTOCOL_MESSAGES_H */
