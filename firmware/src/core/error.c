#include "sudonit/error.h"

const char *sd_strerror(sd_err_t err) {
    switch (err) {
        case SD_OK:
            return "ok";
        case SD_ERR_INVALID:
            return "invalid argument";
        case SD_ERR_IO:
            return "I/O error";
        case SD_ERR_NO_MEM:
            return "out of memory";
        case SD_ERR_TIMEOUT:
            return "timeout";
        case SD_ERR_AGAIN:
            return "not ready (retry)";
        case SD_ERR_UNSUPPORTED:
            return "unsupported";
    }
    return "unknown error";
}
