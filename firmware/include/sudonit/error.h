/* Sudonit firmware — error codes.
 *
 * One small, explicit error type used across the HAL and (later) the protocol
 * and app layers. Every HAL call returns sd_err_t so failures are checkable at
 * each seam (CLAUDE.md: error handling is required for every major feature).
 */
#ifndef SUDONIT_ERROR_H
#define SUDONIT_ERROR_H

typedef enum {
    SD_OK = 0,         /* success */
    SD_ERR_INVALID,    /* bad argument / precondition */
    SD_ERR_IO,         /* peripheral or transport I/O failure */
    SD_ERR_NO_MEM,     /* allocation failed */
    SD_ERR_TIMEOUT,    /* operation timed out */
    SD_ERR_AGAIN,      /* not ready yet; retry */
    SD_ERR_UNSUPPORTED /* not implemented on this backend */
} sd_err_t;

/* Human-readable name for an error code (for logs/diagnostics). Never NULL. */
const char *sd_strerror(sd_err_t err);

#endif /* SUDONIT_ERROR_H */
