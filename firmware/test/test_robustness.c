/* Protocol robustness + property tests — adversarial inputs, no network.
 *
 * test_protocol.c proves the happy path (valid frames/messages round-trip). This
 * file proves the *unhappy* path: the framing and message layers must reject
 * malformed, truncated, oversized, and corrupted input with a defined error code
 * and without hanging, over-reading, or crashing. Everything runs in-process over
 * a socketpair, so the real send/recv/parse code is exercised end to end.
 *
 * Covered:
 *   - framing: bad kind byte, oversized length, buffer-too-small, truncated
 *     header, truncated payload, zero-length frame, and a randomized round-trip
 *     property test over many kinds/sizes.
 *   - audio downlink reassembly: bad SHA, single-byte chunk corruption, a runt
 *     binary chunk, an out-of-range sequence number, a size mismatch, and a
 *     short stream (fewer chunks than advertised) that must time out, not hang.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "sudonit/protocol/framing.h"
#include "sudonit/protocol/messages.h"
#include "sudonit/protocol/sha256.h"
#include "sudonit/hal/transport.h"

static int g_failures;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL: %s (%s:%d)\n", (msg), __FILE__, __LINE__);  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

/* A connected pair of transports plus the raw fds, so a test can either speak
 * the protocol or inject raw bytes on the wire. */
struct wire {
    int fd[2];
    sd_transport_t *a; /* wraps fd[0] */
    sd_transport_t *b; /* wraps fd[1] */
};

static int wire_open(struct wire *w) {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, w->fd) != 0) {
        CHECK(0, "socketpair");
        return -1;
    }
    w->a = sd_transport_wrap_fd(w->fd[0]);
    w->b = sd_transport_wrap_fd(w->fd[1]);
    return 0;
}

static void wire_close(struct wire *w) {
    sd_transport_close(w->a);
    sd_transport_close(w->b);
}

/* Bound the device side's blocking reads so a "missing data" test fails with
 * SD_ERR_TIMEOUT instead of hanging the whole suite. */
static void set_short_rcv_timeout(int fd) {
    struct timeval tv = {.tv_sec = 0, .tv_usec = 150 * 1000}; /* 150 ms */
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static void write_raw(int fd, const void *buf, size_t len) {
    const uint8_t *p = buf;
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, p + off, len - off);
        if (n <= 0) {
            break;
        }
        off += (size_t)n;
    }
}

/* xorshift32: a tiny deterministic PRNG so the property test is reproducible
 * (no external deps, fixed seed). */
static uint32_t prng(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (*s = x);
}

/* --- framing: rejection of malformed input ------------------------------- */

static void test_send_rejects_invalid(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    uint8_t one = 0;
    CHECK(sd_frame_send(w.a, 'X', &one, 1) == SD_ERR_INVALID,
          "send rejects unknown kind");
    /* Oversized length is rejected before any byte is written, so the small
     * payload pointer is never dereferenced. */
    CHECK(sd_frame_send(w.a, SD_KIND_JSON, &one, SD_MAX_FRAME_BYTES + 1) ==
              SD_ERR_INVALID,
          "send rejects oversized length");
    wire_close(&w);
}

static void test_recv_rejects_bad_kind(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    uint8_t hdr[5] = {'X', 0, 0, 0, 0}; /* unknown kind, len 0 */
    write_raw(w.fd[1], hdr, sizeof(hdr));

    char kind = 0;
    uint8_t buf[16];
    size_t len = 0;
    CHECK(sd_frame_recv(w.a, &kind, buf, sizeof(buf), &len) == SD_ERR_IO,
          "recv rejects unknown kind byte");
    wire_close(&w);
}

static void test_recv_rejects_oversized_length(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    uint32_t big = SD_MAX_FRAME_BYTES + 1;
    uint8_t hdr[5] = {SD_KIND_JSON, (uint8_t)(big >> 24), (uint8_t)(big >> 16),
                      (uint8_t)(big >> 8), (uint8_t)big};
    write_raw(w.fd[1], hdr, sizeof(hdr));

    char kind = 0;
    uint8_t buf[16];
    size_t len = 0;
    /* Rejected on the advertised length alone — the (non-existent) 8 MB payload
     * is never read. */
    CHECK(sd_frame_recv(w.a, &kind, buf, sizeof(buf), &len) == SD_ERR_IO,
          "recv rejects oversized advertised length");
    wire_close(&w);
}

static void test_recv_buffer_too_small(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    const char *payload = "0123456789"; /* 10 bytes */
    CHECK(sd_frame_send(w.b, SD_KIND_JSON, (const uint8_t *)payload, 10) == SD_OK,
          "send 10-byte frame");

    char kind = 0;
    uint8_t small[4];
    size_t len = 0;
    CHECK(sd_frame_recv(w.a, &kind, small, sizeof(small), &len) == SD_ERR_NO_MEM,
          "recv reports NO_MEM when frame exceeds caller buffer");
    wire_close(&w);
}

static void test_recv_truncated_header(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    uint8_t partial[2] = {SD_KIND_JSON, 0};
    write_raw(w.fd[1], partial, sizeof(partial));
    sd_transport_close(w.b); /* EOF before the 5-byte header completes (closes fd[1] once) */

    char kind = 0;
    uint8_t buf[16];
    size_t len = 0;
    CHECK(sd_frame_recv(w.a, &kind, buf, sizeof(buf), &len) == SD_ERR_IO,
          "recv fails on truncated header");
    sd_transport_close(w.a);
}

static void test_recv_truncated_payload(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    /* Header promises 100 bytes; only 10 arrive, then EOF. */
    uint8_t hdr[5] = {SD_KIND_JSON, 0, 0, 0, 100};
    write_raw(w.fd[1], hdr, sizeof(hdr));
    uint8_t some[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    write_raw(w.fd[1], some, sizeof(some));
    sd_transport_close(w.b); /* EOF after a partial payload (closes fd[1] once) */

    char kind = 0;
    uint8_t buf[256];
    size_t len = 0;
    CHECK(sd_frame_recv(w.a, &kind, buf, sizeof(buf), &len) == SD_ERR_IO,
          "recv fails on truncated payload");
    sd_transport_close(w.a);
}

static void test_recv_zero_length_frame(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    CHECK(sd_frame_send(w.b, SD_KIND_JSON, NULL, 0) == SD_OK, "send empty frame");
    char kind = 0;
    uint8_t buf[16];
    size_t len = 123;
    CHECK(sd_frame_recv(w.a, &kind, buf, sizeof(buf), &len) == SD_OK &&
              kind == SD_KIND_JSON && len == 0,
          "recv accepts a valid zero-length frame");
    wire_close(&w);
}

/* Property test: any kind + any payload up to a few KB must survive the wire
 * byte-for-byte. Buffered (payloads fit the socket buffer), reproducible seed. */
static void test_framing_roundtrip_property(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    uint32_t seed = 0xC0FFEEu;
    uint8_t out[8192] = {0};
    uint8_t in[8192] = {0};
    int mismatches = 0;

    for (int iter = 0; iter < 250; ++iter) {
        char kind = (prng(&seed) & 1) ? SD_KIND_JSON : SD_KIND_BINARY;
        size_t len = prng(&seed) % (sizeof(out) + 1); /* 0 .. 8192 */
        for (size_t i = 0; i < len; ++i) {
            out[i] = (uint8_t)prng(&seed);
        }
        if (sd_frame_send(w.a, kind, out, len) != SD_OK) {
            ++mismatches;
            continue;
        }
        char rkind = 0;
        size_t rlen = 0;
        if (sd_frame_recv(w.b, &rkind, in, sizeof(in), &rlen) != SD_OK ||
            rkind != kind || rlen != len || memcmp(in, out, len) != 0) {
            ++mismatches;
        }
    }
    CHECK(mismatches == 0, "randomized framing round-trip is lossless");
    wire_close(&w);
}

/* --- audio downlink reassembly: corruption + truncation ------------------ */

/* Send a binary chunk [seq:uint32 BE][data] on `t`. */
static void send_chunk(sd_transport_t *t, uint32_t seq, const uint8_t *data,
                       size_t dlen) {
    uint8_t frame[4 + SD_CHUNK_SIZE];
    frame[0] = (uint8_t)(seq >> 24);
    frame[1] = (uint8_t)(seq >> 16);
    frame[2] = (uint8_t)(seq >> 8);
    frame[3] = (uint8_t)seq;
    if (dlen > 0) {
        memcpy(frame + 4, data, dlen);
    }
    sd_frame_send(t, SD_KIND_BINARY, frame, 4 + dlen);
}

/* Send a valid audio_begin describing `size`/`chunks`/`sha`. */
static void send_begin(sd_transport_t *t, size_t size, uint32_t chunks,
                       const char *sha) {
    char begin[256];
    snprintf(begin, sizeof(begin),
             "{\"type\":\"audio_begin\",\"audio_id\":\"x\",\"size\":%zu,"
             "\"chunks\":%u,\"sha256\":\"%s\",\"format\":\"pcm_s16le\","
             "\"sample_rate\":16000,\"channels\":1}",
             size, (unsigned)chunks, sha);
    sd_frame_send(t, SD_KIND_JSON, (const uint8_t *)begin, strlen(begin));
}

static void send_end(sd_transport_t *t) {
    const char *end = "{\"type\":\"audio_end\",\"audio_id\":\"x\"}";
    sd_frame_send(t, SD_KIND_JSON, (const uint8_t *)end, strlen(end));
}

/* Receive the audio_begin on the device side and hand back its JSON. */
static sd_err_t recv_begin(sd_transport_t *device, char *json, size_t cap) {
    sd_msg_type_t type = SD_MSG_UNKNOWN;
    sd_err_t err = sd_msg_recv(device, &type, json, cap);
    if (err == SD_OK && type != SD_MSG_AUDIO_BEGIN) {
        return SD_ERR_IO;
    }
    return err;
}

static void test_audio_body_bad_sha(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    uint8_t pcm[6000];
    for (size_t i = 0; i < sizeof(pcm); ++i) pcm[i] = (uint8_t)(i * 3 + 1);
    uint32_t chunks = (uint32_t)((sizeof(pcm) + SD_CHUNK_SIZE - 1) / SD_CHUNK_SIZE);

    /* Advertise a wrong (but well-formed) SHA. */
    const char *wrong_sha =
        "0000000000000000000000000000000000000000000000000000000000000000";
    send_begin(w.b, sizeof(pcm), chunks, wrong_sha);
    for (uint32_t s = 0; s < chunks; ++s) {
        size_t start = (size_t)s * SD_CHUNK_SIZE;
        size_t dlen = (start + SD_CHUNK_SIZE <= sizeof(pcm)) ? SD_CHUNK_SIZE
                                                            : sizeof(pcm) - start;
        send_chunk(w.b, s, pcm + start, dlen);
    }
    send_end(w.b);

    char begin[512];
    CHECK(recv_begin(w.a, begin, sizeof(begin)) == SD_OK, "classify audio_begin");
    uint8_t out[6000];
    CHECK(sd_msg_recv_audio_body(w.a, chunks, sizeof(pcm), wrong_sha, out,
                                 sizeof(out)) == SD_ERR_IO,
          "audio body rejects a SHA mismatch");
    wire_close(&w);
}

static void test_audio_body_chunk_corruption(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    uint8_t pcm[6000];
    for (size_t i = 0; i < sizeof(pcm); ++i) pcm[i] = (uint8_t)(i * 5 + 2);
    uint32_t chunks = (uint32_t)((sizeof(pcm) + SD_CHUNK_SIZE - 1) / SD_CHUNK_SIZE);
    char sha[65];
    sd_sha256_hex(pcm, sizeof(pcm), sha); /* SHA of the *intended* data */

    send_begin(w.b, sizeof(pcm), chunks, sha);
    for (uint32_t s = 0; s < chunks; ++s) {
        size_t start = (size_t)s * SD_CHUNK_SIZE;
        size_t dlen = (start + SD_CHUNK_SIZE <= sizeof(pcm)) ? SD_CHUNK_SIZE
                                                            : sizeof(pcm) - start;
        uint8_t tmp[SD_CHUNK_SIZE];
        memcpy(tmp, pcm + start, dlen);
        if (s == 0) tmp[10] ^= 0xFF; /* flip one byte in transit */
        send_chunk(w.b, s, tmp, dlen);
    }
    send_end(w.b);

    char begin[512];
    CHECK(recv_begin(w.a, begin, sizeof(begin)) == SD_OK, "classify audio_begin");
    uint8_t out[6000];
    CHECK(sd_msg_recv_audio_body(w.a, chunks, sizeof(pcm), sha, out,
                                 sizeof(out)) == SD_ERR_IO,
          "audio body detects single-byte chunk corruption via SHA");
    wire_close(&w);
}

static void test_audio_body_runt_chunk(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    /* A binary frame shorter than the 4-byte sequence header is malformed. */
    uint8_t runt[2] = {0, 0};
    sd_frame_send(w.b, SD_KIND_BINARY, runt, sizeof(runt));

    uint8_t out[16];
    char sha[65];
    sd_sha256_hex((const uint8_t *)"", 0, sha);
    CHECK(sd_msg_recv_audio_body(w.a, 1, 0, sha, out, sizeof(out)) == SD_ERR_IO,
          "audio body rejects a runt (<4 byte) binary chunk");
    wire_close(&w);
}

static void test_audio_body_out_of_range_seq(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    uint8_t pcm[100];
    for (size_t i = 0; i < sizeof(pcm); ++i) pcm[i] = (uint8_t)i;
    char sha[65];
    sd_sha256_hex(pcm, sizeof(pcm), sha);

    /* One chunk, but with a wildly out-of-range sequence number: its byte offset
     * (seq * CHUNK_SIZE) lands far past the destination buffer. */
    send_chunk(w.b, 1000, pcm, sizeof(pcm));

    uint8_t out[100];
    CHECK(sd_msg_recv_audio_body(w.a, 1, sizeof(pcm), sha, out, sizeof(out)) ==
              SD_ERR_IO,
          "audio body rejects an out-of-range sequence number (no overflow)");
    wire_close(&w);
}

static void test_audio_body_size_mismatch(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;

    uint8_t pcm[100];
    for (size_t i = 0; i < sizeof(pcm); ++i) pcm[i] = (uint8_t)(i + 9);
    char sha[65];
    sd_sha256_hex(pcm, sizeof(pcm), sha);

    /* Send the real single chunk + end, but claim a larger total size. */
    send_chunk(w.b, 0, pcm, sizeof(pcm));
    send_end(w.b);

    uint8_t out[256];
    CHECK(sd_msg_recv_audio_body(w.a, 1, sizeof(pcm) + 50, sha, out,
                                 sizeof(out)) == SD_ERR_IO,
          "audio body rejects a declared/actual size mismatch");
    wire_close(&w);
}

static void test_audio_body_short_stream_times_out(void) {
    struct wire w;
    if (wire_open(&w) != 0) return;
    set_short_rcv_timeout(w.fd[0]); /* device side must not hang */

    uint8_t pcm[100] = {0};
    char sha[65];
    sd_sha256_hex(pcm, sizeof(pcm), sha);

    /* Advertise 3 chunks but send only one, then stop sending. */
    send_chunk(w.b, 0, pcm, sizeof(pcm));

    uint8_t out[256];
    CHECK(sd_msg_recv_audio_body(w.a, 3, sizeof(pcm), sha, out, sizeof(out)) ==
              SD_ERR_TIMEOUT,
          "audio body times out (not hangs) when chunks are missing");
    wire_close(&w);
}

int main(void) {
    test_send_rejects_invalid();
    test_recv_rejects_bad_kind();
    test_recv_rejects_oversized_length();
    test_recv_buffer_too_small();
    test_recv_truncated_header();
    test_recv_truncated_payload();
    test_recv_zero_length_frame();
    test_framing_roundtrip_property();

    test_audio_body_bad_sha();
    test_audio_body_chunk_corruption();
    test_audio_body_runt_chunk();
    test_audio_body_out_of_range_seq();
    test_audio_body_size_mismatch();
    test_audio_body_short_stream_times_out();

    if (g_failures == 0) {
        printf("all robustness tests passed\n");
        return 0;
    }
    fprintf(stderr, "%d robustness test(s) failed\n", g_failures);
    return 1;
}
