/* Host tests for the protocol port — no network required.
 *
 * Framing/message round-trips run over a socketpair, so both ends of the wire
 * exercise the real send/recv/parse path in-process. SHA-256 and the JSON
 * extractor are checked against known values. The live interop with the Python
 * server is a separate manual step (firmware/test/run_interop.sh).
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sudonit/protocol/framing.h"
#include "sudonit/protocol/json.h"
#include "sudonit/protocol/messages.h"
#include "sudonit/protocol/sha256.h"
#include "sudonit/hal/transport.h"

static int g_failures;

#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "FAIL: %s (%s:%d)\n", (msg), __FILE__, __LINE__); \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

static void test_sha256(void) {
    /* FIPS 180-4 vector: sha256("abc"). */
    char hex[65];
    sd_sha256_hex((const uint8_t *)"abc", 3, hex);
    CHECK(strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61"
                      "f20015ad") == 0,
          "sha256(\"abc\")");

    /* sha256("") */
    sd_sha256_hex((const uint8_t *)"", 0, hex);
    CHECK(strcmp(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b"
                      "7852b855") == 0,
          "sha256(\"\")");
}

static void test_json(void) {
    char out[128];
    CHECK(sd_json_get_string("{\"type\":\"ai_response\",\"text\":\"hello world\"}",
                             "type", out, sizeof(out)) == SD_OK &&
              strcmp(out, "ai_response") == 0,
          "json type");
    CHECK(sd_json_get_string("{\"text\":\"a \\\"quote\\\" and\\nnewline\"}", "text",
                             out, sizeof(out)) == SD_OK &&
              strcmp(out, "a \"quote\" and\nnewline") == 0,
          "json unescape");
    CHECK(sd_json_get_string("{\"text\":\"caf\\u00e9\"}", "text", out, sizeof(out)) ==
                  SD_OK &&
              strcmp(out, "caf\xc3\xa9") == 0,
          "json \\u -> utf8");
    CHECK(sd_json_get_string("{\"type\":\"pong\"}", "missing", out, sizeof(out)) ==
              SD_ERR_INVALID,
          "json missing key");
}

static void test_framing_roundtrip(void) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        CHECK(0, "socketpair");
        return;
    }
    sd_transport_t *a = sd_transport_wrap_fd(sv[0]);
    sd_transport_t *b = sd_transport_wrap_fd(sv[1]);

    const char *json = "{\"type\":\"ping\"}";
    CHECK(sd_frame_send(a, SD_KIND_JSON, (const uint8_t *)json, strlen(json)) == SD_OK,
          "send json frame");
    char kind = 0;
    uint8_t buf[256];
    size_t len = 0;
    CHECK(sd_frame_recv(b, &kind, buf, sizeof(buf), &len) == SD_OK, "recv json frame");
    CHECK(kind == SD_KIND_JSON && len == strlen(json) &&
              memcmp(buf, json, len) == 0,
          "json frame matches");

    /* Binary frame round-trip. */
    uint8_t payload[10] = {0, 0, 0, 5, 1, 2, 3, 4, 5};
    CHECK(sd_frame_send(a, SD_KIND_BINARY, payload, sizeof(payload)) == SD_OK,
          "send binary frame");
    CHECK(sd_frame_recv(b, &kind, buf, sizeof(buf), &len) == SD_OK, "recv binary frame");
    CHECK(kind == SD_KIND_BINARY && len == sizeof(payload) &&
              memcmp(buf, payload, len) == 0,
          "binary frame matches");

    sd_transport_close(a);
    sd_transport_close(b);
}

/* A tiny in-process "phone": reads an image_begin/chunks/image_end, then sends
 * ai_response + play_audio — enough to exercise the message layer end-to-end. */
struct fake_phone_args {
    sd_transport_t *t;
    int ok;
};

static void *fake_phone(void *arg) {
    struct fake_phone_args *a = arg;
    char kind = 0;
    uint8_t buf[SD_CHUNK_SIZE + 64];
    size_t len = 0;

    int saw_begin = 0, saw_end = 0, chunks = 0;
    while (sd_frame_recv(a->t, &kind, buf, sizeof(buf), &len) == SD_OK) {
        if (kind == SD_KIND_JSON) {
            buf[len] = '\0';
            char type[32];
            if (sd_json_get_string((const char *)buf, "type", type, sizeof(type)) ==
                SD_OK) {
                if (strcmp(type, "image_begin") == 0) saw_begin = 1;
                else if (strcmp(type, "image_end") == 0) { saw_end = 1; break; }
            }
        } else {
            ++chunks;
        }
    }
    a->ok = saw_begin && saw_end && chunks >= 1;

    const char *resp = "{\"type\":\"ai_response\",\"text\":\"a test scene\"}";
    const char *play = "{\"type\":\"play_audio\",\"content\":\"a test scene\"}";
    sd_frame_send(a->t, SD_KIND_JSON, (const uint8_t *)resp, strlen(resp));
    sd_frame_send(a->t, SD_KIND_JSON, (const uint8_t *)play, strlen(play));
    return NULL;
}

static void test_message_layer_roundtrip(void) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        CHECK(0, "socketpair");
        return;
    }
    sd_transport_t *device = sd_transport_wrap_fd(sv[0]);
    struct fake_phone_args pa = {sd_transport_wrap_fd(sv[1]), 0};

    pthread_t th;
    pthread_create(&th, NULL, fake_phone, &pa);

    const uint8_t image[5000] = {0}; /* spans multiple chunks */
    CHECK(sd_msg_send_image(device, "capture", image, sizeof(image),
                            "image/x-mock-rgb") == SD_OK,
          "send image");

    sd_msg_type_t type = SD_MSG_UNKNOWN;
    char text[256];
    int got_response = 0, done = 0;
    while (!done) {
        if (sd_msg_recv(device, &type, text, sizeof(text)) != SD_OK) {
            CHECK(0, "msg recv");
            break;
        }
        if (type == SD_MSG_AI_RESPONSE) {
            got_response = strcmp(text, "a test scene") == 0;
        } else if (type == SD_MSG_PLAY_AUDIO) {
            done = 1;
        }
    }

    pthread_join(th, NULL);
    CHECK(pa.ok, "phone received begin+chunks+end");
    CHECK(got_response, "device parsed ai_response text");

    sd_transport_close(device);
    sd_transport_close(pa.t);
}

int main(void) {
    test_sha256();
    test_json();
    test_framing_roundtrip();
    test_message_layer_roundtrip();

    if (g_failures == 0) {
        printf("all protocol tests passed\n");
        return 0;
    }
    fprintf(stderr, "%d protocol test(s) failed\n", g_failures);
    return 1;
}
