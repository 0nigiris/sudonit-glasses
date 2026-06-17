/* Host transport backend: a blocking TCP client.
 *
 * This is the dev-machine implementation of the transport HAL — the Wi-Fi
 * data-plane stand-in. It connects to the Python phone server over TCP and
 * moves bytes all-or-nothing. On the ESP32, transport_wifi.c implements the
 * same four functions over a Wi-Fi TCP socket; nothing above this changes.
 */
#include "sudonit/hal/transport.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = NULL;
    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
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

sd_transport_t *sd_transport_wrap_fd(int fd) {
    sd_transport_t *t = calloc(1, sizeof(*t));
    if (t) {
        t->fd = fd;
    }
    return t;
}
