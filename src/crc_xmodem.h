#ifndef CRC_XMODEM_H
#define CRC_XMODEM_H


#define CRC_XMODEM_INIT_VAL 0x0000U


#ifdef AVR
#include <util/crc16.h> // use optimized avr crc function
#define crc_xmodem_update(a, b) _crc_xmodem_update(a, b)
#else
#include "crc_xmodem_generic.h" // use generic crc function
#define crc_xmodem_update(a, b) generic_crc_xmodem_update(a, b)
#endif

#endif
