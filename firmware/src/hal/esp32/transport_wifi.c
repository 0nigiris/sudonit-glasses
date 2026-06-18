/* ESP32 transport backend — TCP client over the Wi-Fi data plane.
 *
 * This is the on-target counterpart of the host transport_tcp.c. LwIP (the
 * ESP-IDF network stack) exposes a BSD-compatible socket API, so this is the
 * same blocking, all-or-nothing TCP client as the host build; only the headers
 * differ (lwip/sockets.h, lwip/netdb.h). Nothing above this layer changes — the
 * framing/protocol code is byte-identical on host and device.
 *
 * Bringing Wi-Fi up (associating with the AP, getting an IP) is a separate
 * concern handled by net_esp.c (sd_net_start); by the time this connect runs the
 * interface is expected to already have an address.
 */
#include "sudonit/hal/transport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwip/netdb.h"
#include "lwip/sockets.h"

struct sd_transport {
    int fd;
};

sd_err_t sd_transport_connect(sd_transport_t **out, const char *host, uint16_t port) {
    if (!out || !host) {
        return SD_ERR_INVALID;
    }
    *out = NULL;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%u", port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* IPv4 only on the data plane for now */
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = NULL;
    if (getaddrinfo(host, port_str, &hints, &res) != 0 || !res) {
        return SD_ERR_IO;
    }

    int fd = -1;
    for (struct addrinfo *ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd < 0) {
        return SD_ERR_IO;
    }

    sd_transport_t *t = calloc(1, sizeof(*t));
    if (!t) {
        close(fd);
        return SD_ERR_NO_MEM;
    }
    t->fd = fd;
    *out = t;
    return SD_OK;
}

sd_err_t sd_transport_send(sd_transport_t *t, const uint8_t *buf, size_t len) {
    if (!t || (!buf && len > 0)) {
        return SD_ERR_INVALID;
    }
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(t->fd, buf + sent, len - sent, 0);
        if (n <= 0) {
            return SD_ERR_IO;
        }
        sent += (size_t)n;
    }
    return SD_OK;
}

sd_err_t sd_transport_recv(sd_transport_t *t, uint8_t *buf, size_t len) {
    if (!t || (!buf && len > 0)) {
        return SD_ERR_INVALID;
    }
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(t->fd, buf + got, len - got, 0);
        if (n <= 0) {
            return SD_ERR_IO; /* peer closed or error */
        }
        got += (size_t)n;
    }
    return SD_OK;
}

void sd_transport_close(sd_transport_t *t) {
    if (t) {
        if (t->fd >= 0) {
            close(t->fd);
        }
        free(t);
    }
}
