#ifndef CRC_XMODEM_GENERIC_H
#define CRC_XMODEM_GENERIC_H

#include <stdint.h>


uint16_t generic_crc_xmodem_update(uint16_t crc, uint8_t data);

#endif
