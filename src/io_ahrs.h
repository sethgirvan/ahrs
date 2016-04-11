#ifndef IO_AHRS_H
#define IO_AHRS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>

void io_ahrs_init(char const *path);

void io_ahrs_clean();

/**
 * returns 0 on success
 */
int io_ahrs_recv_start(int (*handler)());

void io_ahrs_recv_stop();

/**
 * io with this is blocking, so one might use normal stdio functions directly
 * on it when they are willing to wait, eg sending initial configuration data,
 * but to be able to handle asynchronous io, one should use io_ahrs_..._start,
 * which allows us to process data on the fly and avoid polling.
 */
extern FILE *io_ahrs;

/**
 * Causes io_ahrs_tripbuf_read to return index that was most recently
 * 'submitted' by io_ahrs_tripbuf_offer.
 *
 * This should only be run between complete 'uses' of the data, to avoid using
 * disparate data.
 *
 * Unfortunately, since there is no way to have a lock-free/interruptable
 * triple buffer implementation on the avr, this has to be platform-specific.
 * A lock-free ring buffer would not handle cases when the producer is faster
 * than the consumer well.
 */
bool io_ahrs_tripbuf_update();

/**
 * Makes the current write index available to io_ahrs_tripbuf_update, and
 * changes the value returned by io_ahrs_tripbuf_write.
 */
void io_ahrs_tripbuf_offer();

/**
 * returns the index of the buffer the data consumer should read from. Only
 * changes if io_ahrs_tripbuf_update is called and returns true.
 */
unsigned char io_ahrs_tripbuf_read();

/**
 * returns the index of the buffer the data producer should write to. Only
 * changes when io_ahrs_tripbuf_offer is called.
 */
unsigned char io_ahrs_tripbuf_write();

#ifdef __cplusplus
}
#endif

#endif
