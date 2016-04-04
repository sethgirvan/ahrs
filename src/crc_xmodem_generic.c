#include <limits.h>
#include <stddef.h>

#include "crc_xmodem_generic.h"


#define CRC16CCIT_XMODEM_POLY 0x1021U

/**
 * Update a crc with 8 bits of additional data. This is unoptimized and meant
 * to be used for testing on computer. On avr, one should use the optimized
 * _crc_xmodem_update() in util/crc16.h
 */
uint16_t generic_crc_xmodem_update(uint16_t crc, uint8_t data)
{
	crc ^= (uint16_t)data << 8; // xor in the new data
	
	for (uint_fast8_t i = 8; i--;)
	{
		if (crc & 0x8000U) // is high bit 1?
		{
			crc = (crc << 1) ^ CRC16CCIT_XMODEM_POLY;
		}
		else
		{
			crc <<= 1;
		}
	}

	return crc;
}
