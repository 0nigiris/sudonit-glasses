/* Sudonit protocol — wire framing (C port).
 *
 * Byte-for-byte compatible with protocol/framing.py. Frame on the wire:
 *
 *     [kind: 1 byte][length: uint32 big-endian][payload]
 *
 *   kind 'J' (0x4A) -> JSON control message
 *   kind 'B' (0x42) -> binary chunk: [seq: uint32 BE][data]
 *
 * Runs over the transport HAL, so the same code works over host TCP today and
 * ESP32 Wi-Fi later.
 */
#ifndef SUDONIT_PROTOCOL_FRAMING_H
#define SUDONIT_PROTOCOL_FRAMING_H

#include <stddef.h>
#include <stdint.h>

#include "sudonit/error.h"
#include "sudonit/hal/transport.h"

#define SD_KIND_JSON 'J'
#define SD_KIND_BINARY 'B'
#define SD_MAX_FRAME_BYTES (8u * 1024u * 1024u)

/* Send one frame. */
sd_err_t sd_frame_send(sd_transport_t *t, char kind, const uint8_t *payload, size_t len);

/* Receive one frame into the caller's buffer.
 *   kind_out  : 'J' or 'B'
 *   buf/cap   : destination; SD_ERR_NO_MEM if the frame exceeds cap
 *   len_out   : actual payload length
 * The C side only ever receives small control frames, so a fixed caller buffer
 * is sufficient and we avoid heap allocation in the framing layer.
 */
sd_err_t sd_frame_recv(sd_transport_t *t, char *kind_out, uint8_t *buf, size_t cap,
                       size_t *len_out);

#endif /* SUDONIT_PROTOCOL_FRAMING_H */
