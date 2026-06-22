#include "sudonit/protocol/messages.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sudonit/protocol/framing.h"
#include "sudonit/protocol/json.h"
#include "sudonit/protocol/sha256.h"

sd_err_t sd_msg_send_ping(sd_transport_t *t) {
    static const char ping[] = "{\"type\":\"ping\"}";
    return sd_frame_send(t, SD_KIND_JSON, (const uint8_t *)ping, strlen(ping));
}

sd_err_t sd_msg_send_image(sd_transport_t *t, const char *image_id,
                           const uint8_t *data, size_t len, const char *media_type) {
    if (!t || !image_id || (!data && len > 0) || !media_type) {
        return SD_ERR_INVALID;
    }

    size_t chunks = (len + SD_CHUNK_SIZE - 1) / SD_CHUNK_SIZE;
    if (chunks == 0) {
        chunks = 1; /* matches the Python sender: at least one (empty) chunk */
    }

    char sha_hex[65];
    sd_sha256_hex(data, len, sha_hex);

    /* image_begin — keys can be in any order; the phone parses by name. */
    char begin[256];
    int n = snprintf(begin, sizeof(begin),
                     "{\"type\":\"image_begin\",\"image_id\":\"%s\",\"size\":%zu,"
                     "\"chunks\":%zu,\"sha256\":\"%s\",\"media_type\":\"%s\"}",
                     image_id, len, chunks, sha_hex, media_type);
    if (n < 0 || (size_t)n >= sizeof(begin)) {
        return SD_ERR_INVALID;
    }
    sd_err_t err = sd_frame_send(t, SD_KIND_JSON, (const uint8_t *)begin, (size_t)n);
    if (err != SD_OK) {
        return err;
    }

    /* Binary chunks: [seq: uint32 BE][data slice]. Heap-allocated rather than a
     * 4 KB stack array — on the ESP32 the main task stack is only a few KB, so a
     * stack buffer this size would overflow it. */
    uint8_t *frame = malloc(4 + SD_CHUNK_SIZE);
    if (!frame) {
        return SD_ERR_NO_MEM;
    }
    for (size_t seq = 0; seq < chunks; ++seq) {
        size_t start = seq * SD_CHUNK_SIZE;
        size_t clen = (start < len) ? (len - start) : 0;
        if (clen > SD_CHUNK_SIZE) clen = SD_CHUNK_SIZE;
        frame[0] = (uint8_t)(seq >> 24);
        frame[1] = (uint8_t)(seq >> 16);
        frame[2] = (uint8_t)(seq >> 8);
        frame[3] = (uint8_t)seq;
        if (clen > 0) {
            memcpy(frame + 4, data + start, clen);
        }
        err = sd_frame_send(t, SD_KIND_BINARY, frame, 4 + clen);
        if (err != SD_OK) {
            free(frame);
            return err;
        }
    }
    free(frame);

    /* image_end */
    char end[128];
    n = snprintf(end, sizeof(end), "{\"type\":\"image_end\",\"image_id\":\"%s\"}",
                 image_id);
    if (n < 0 || (size_t)n >= sizeof(end)) {
        return SD_ERR_INVALID;
    }
    return sd_frame_send(t, SD_KIND_JSON, (const uint8_t *)end, (size_t)n);
}

sd_err_t sd_msg_recv(sd_transport_t *t, sd_msg_type_t *type_out, char *text_out,
                     size_t text_cap) {
    if (text_out && text_cap > 0) {
        text_out[0] = '\0';
    }

    /* Heap, not stack: the largest control frame is several KB and the ESP32
     * main-task stack is only ~3.5 KB — an 8 KB stack buffer would overflow it.
     * Matches the chunk-buffer policy used by the send/audio paths in this file. */
    enum { SD_MSG_RECV_CAP = 8192 };
    uint8_t *buf = malloc(SD_MSG_RECV_CAP);
    if (!buf) {
        return SD_ERR_NO_MEM;
    }

    char kind = 0;
    size_t len = 0;
    sd_err_t err = sd_frame_recv(t, &kind, buf, SD_MSG_RECV_CAP - 1, &len);
    if (err != SD_OK) {
        free(buf);
        return err;
    }
    if (kind != SD_KIND_JSON) {
        free(buf);
        return SD_ERR_IO; /* device does not expect inbound binary frames */
    }
    buf[len] = '\0';
    const char *json = (const char *)buf;

    char type[32];
    if (sd_json_get_string(json, "type", type, sizeof(type)) != SD_OK) {
        free(buf);
        return SD_ERR_IO;
    }

    sd_msg_type_t mtype = SD_MSG_UNKNOWN;
    const char *text_key = NULL;
    if (strcmp(type, "pong") == 0) {
        mtype = SD_MSG_PONG;
    } else if (strcmp(type, "device_info") == 0) {
        mtype = SD_MSG_DEVICE_INFO;
    } else if (strcmp(type, "ai_response") == 0) {
        mtype = SD_MSG_AI_RESPONSE;
        text_key = "text";
    } else if (strcmp(type, "play_audio") == 0) {
        mtype = SD_MSG_PLAY_AUDIO;
        text_key = "content";
    } else if (strcmp(type, "error") == 0) {
        mtype = SD_MSG_ERROR;
        text_key = "message";
    } else if (strcmp(type, "audio_begin") == 0) {
        mtype = SD_MSG_AUDIO_BEGIN;
    }

    if (mtype == SD_MSG_AUDIO_BEGIN && text_out && text_cap > 0) {
        /* Hand back the raw begin JSON; the caller reads the numeric fields. */
        snprintf(text_out, text_cap, "%s", json);
    } else if (text_key && text_out && text_cap > 0) {
        sd_json_get_string(json, text_key, text_out, text_cap); /* best-effort */
    }
    if (type_out) *type_out = mtype;
    free(buf);
    return SD_OK;
}

sd_err_t sd_msg_recv_audio_body(sd_transport_t *t, uint32_t chunks, uint32_t size,
                                const char *sha_hex, uint8_t *pcm, size_t pcm_cap) {
    if (!t || !sha_hex || (!pcm && size > 0) || pcm_cap < size) {
        return SD_ERR_INVALID;
    }

    /* Heap, not stack: a chunk frame is 4 + SD_CHUNK_SIZE bytes — too large for
     * the ESP32 main-task stack (matches the image-send buffer policy). */
    uint8_t *frame = malloc(4 + SD_CHUNK_SIZE);
    if (!frame) {
        return SD_ERR_NO_MEM;
    }

    sd_err_t err = SD_OK;
    size_t total = 0;
    for (uint32_t i = 0; i < chunks; ++i) {
        char kind = 0;
        size_t len = 0;
        err = sd_frame_recv(t, &kind, frame, 4 + SD_CHUNK_SIZE, &len);
        if (err != SD_OK) {
            goto done;
        }
        if (kind != SD_KIND_BINARY || len < 4) {
            err = SD_ERR_IO;
            goto done;
        }
        uint32_t seq = ((uint32_t)frame[0] << 24) | ((uint32_t)frame[1] << 16) |
                       ((uint32_t)frame[2] << 8) | (uint32_t)frame[3];
        size_t dlen = len - 4;
        size_t off = (size_t)seq * SD_CHUNK_SIZE;
        if (off + dlen > pcm_cap) {
            err = SD_ERR_IO; /* out-of-range sequence number */
            goto done;
        }
        if (dlen > 0) {
            memcpy(pcm + off, frame + 4, dlen);
        }
        total += dlen;
    }

    /* The closing audio_end control frame. */
    {
        char kind = 0;
        size_t len = 0;
        err = sd_frame_recv(t, &kind, frame, 4 + SD_CHUNK_SIZE, &len);
        if (err != SD_OK) {
            goto done;
        }
        if (kind != SD_KIND_JSON) {
            err = SD_ERR_IO;
            goto done;
        }
    }

    if (total != size) {
        err = SD_ERR_IO;
        goto done;
    }
    char hex[65];
    sd_sha256_hex(pcm, size, hex);
    if (strcmp(hex, sha_hex) != 0) {
        err = SD_ERR_IO; /* corrupted in transit */
    }

done:
    free(frame);
    return err;
}
