#include "sudonit/protocol/framing.h"

sd_err_t sd_frame_send(sd_transport_t *t, char kind, const uint8_t *payload, size_t len) {
    if (kind != SD_KIND_JSON && kind != SD_KIND_BINARY) {
        return SD_ERR_INVALID;
    }
    if (len > SD_MAX_FRAME_BYTES) {
        return SD_ERR_INVALID;
    }
    uint8_t hdr[5];
    hdr[0] = (uint8_t)kind;
    hdr[1] = (uint8_t)(len >> 24);
    hdr[2] = (uint8_t)(len >> 16);
    hdr[3] = (uint8_t)(len >> 8);
    hdr[4] = (uint8_t)len;

    sd_err_t err = sd_transport_send(t, hdr, sizeof(hdr));
    if (err != SD_OK) {
        return err;
    }
    if (len == 0) {
        return SD_OK;
    }
    return sd_transport_send(t, payload, len);
}

sd_err_t sd_frame_recv(sd_transport_t *t, char *kind_out, uint8_t *buf, size_t cap,
                       size_t *len_out) {
    uint8_t hdr[5];
    sd_err_t err = sd_transport_recv(t, hdr, sizeof(hdr));
    if (err != SD_OK) {
        return err;
    }
    char kind = (char)hdr[0];
    if (kind != SD_KIND_JSON && kind != SD_KIND_BINARY) {
        return SD_ERR_IO;
    }
    size_t len = (size_t)hdr[1] << 24 | (size_t)hdr[2] << 16 |
                 (size_t)hdr[3] << 8 | (size_t)hdr[4];
    if (len > SD_MAX_FRAME_BYTES) {
        return SD_ERR_IO;
    }
    if (len > cap) {
        return SD_ERR_NO_MEM;
    }
    if (len > 0) {
        err = sd_transport_recv(t, buf, len);
        if (err != SD_OK) {
            return err;
        }
    }
    if (kind_out) *kind_out = kind;
    if (len_out) *len_out = len;
    return SD_OK;
}
