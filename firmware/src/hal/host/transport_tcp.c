/* Host transport backend: a blocking TCP client with bounded waits.
 *
 * This is the dev-machine implementation of the transport HAL — the Wi-Fi
 * data-plane stand-in. It connects to the Python phone server over TCP and
 * moves bytes all-or-nothing. On the ESP32, transport_wifi.c implements the
 * same four functions over a Wi-Fi TCP socket; nothing above this changes.
 *
 * Every wait is bounded: connect uses a non-blocking attempt with select(), and
 * recv/send use socket receive/send timeouts. A stalled or unreachable phone
 * therefore yields SD_ERR_TIMEOUT instead of hanging the device forever — the
 * single most important robustness property for unattended hardware.
 */
#include "sudonit/hal/transport.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/* Bounded waits. Generous enough for a real AI round trip over Wi-Fi, short
 * enough that a dead peer is detected rather than hanging indefinitely. */
#define SD_CONNECT_TIMEOUT_MS 5000
#define SD_IO_TIMEOUT_MS 15000

struct sd_transport {
    int fd;
};

static void set_io_timeouts(int fd) {
    struct timeval tv = {
        .tv_sec = SD_IO_TIMEOUT_MS / 1000,
        .tv_usec = (SD_IO_TIMEOUT_MS % 1000) * 1000,
    };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

/* Connect with a timeout: switch the socket to non-blocking, start the connect,
 * and wait for writability (or error) for at most SD_CONNECT_TIMEOUT_MS. Returns
 * 0 on success, -1 otherwise. Leaves the socket back in blocking mode. */
static int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t alen) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }

    int rc = connect(fd, addr, alen);
    if (rc != 0) {
        if (errno != EINPROGRESS) {
            return -1;
        }
        fd_set wf;
        FD_ZERO(&wf);
        FD_SET(fd, &wf);
        struct timeval tv = {
            .tv_sec = SD_CONNECT_TIMEOUT_MS / 1000,
            .tv_usec = (SD_CONNECT_TIMEOUT_MS % 1000) * 1000,
        };
        if (select(fd + 1, NULL, &wf, NULL, &tv) <= 0) {
            return -1; /* timed out or select error */
        }
        int soerr = 0;
        socklen_t len = sizeof(soerr);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &len) < 0 || soerr != 0) {
            return -1; /* connect failed */
        }
    }

    fcntl(fd, F_SETFL, flags); /* restore blocking mode for all-or-nothing I/O */
    return 0;
}

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
        if (connect_with_timeout(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd < 0) {
        return SD_ERR_IO;
    }

    set_io_timeouts(fd);

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
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return SD_ERR_TIMEOUT;
        }
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
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return SD_ERR_TIMEOUT; /* peer stalled mid-frame */
        }
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
