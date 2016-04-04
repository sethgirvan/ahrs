#ifndef AHRS_UTIL_H
#define AHRS_UTIL_H

#include <stdint.h>


uint16_t crc16(char *data, size_t n);

size_t ahrs_write_raw(char *datagram, size_t n);

size_t ahrs_write(char *data, uint_fast16_t len);

#endif
