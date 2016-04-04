#include <stdint.h>
#include <stdio.h>

#include "crc_xmodem.h"
#include "io_ahrs.h"

/**
 * returns crc16-xmodem checksum of n bytes of data
 */
uint16_t crc16(char *data, size_t n)
{
	uint16_t crc = CRC_XMODEM_INIT_VAL;
	for (size_t i = 0; i < n; ++i)
	{
		crc = crc_xmodem_update(crc, data[i]);
	}
	return crc;
}

/**
 * writes n bytes from data to io_ahrs, or until error or EOF
 *
 * returns number of bytes written, which equals n if all data was written
 */
size_t ahrs_write_raw(char *datagram, size_t n)
{
	if (!io_ahrs)
	{
		// ahrs_init needs to be called to init io_ahrs
		return 0;
	}

	size_t nwrit = fwrite(datagram, 1, n, io_ahrs);
	fflush(io_ahrs); // this is probably a null operation on avr
	return nwrit;
}

/**
 * helper function for ahrs_write writes datagram to io_ahrs until EOF or
 * error
 *
 * returns number of bytes written does not flush io_ahrs
 */
static size_t write_datagram(uint_fast16_t bytecount,
		char *data, uint_fast16_t crc)
{
	size_t nwrit = 0;
	if (putc(bytecount >> 8, io_ahrs) == EOF)
	{
		return nwrit;
	}
	++nwrit;
	if (putc(bytecount, io_ahrs) == EOF)
	{
		return nwrit;
	}
	++nwrit;

	nwrit += fwrite(data, 1, bytecount - 4, io_ahrs);
	if (nwrit != bytecount - 2U) // there should only be 2 bytes left to write
	{
		return nwrit;
	}
	
	if (putc(crc >> 8, io_ahrs) == EOF)
	{
		return nwrit;
	}
	++nwrit;
	if(putc(crc, io_ahrs) == EOF)
	{
		return nwrit;
	}
	++nwrit;

	return nwrit;
}

/**
 * computes crc16-xmodem checksum of concatenation of two byte byte count
 * prefix followed by bytecount - 4 bytes of data
 */
static uint16_t crc16_bc(uint_fast16_t bytecount, char *data)
{
	uint16_t crc = CRC_XMODEM_INIT_VAL;
	crc = crc_xmodem_update(crc, bytecount >> 8);
	crc = crc_xmodem_update(crc, bytecount);
	for (uint16_t i = 0; i < bytecount - 4; ++i)
	{
		crc = crc_xmodem_update(crc, data[i]);
	}
	return crc;
}

/**
 * creates a datagram from len bytes of data by prepending 2 length bytes of
 * value len + 4, and appending 2 checksum bytes of the crc of both the length
 * and data bytes, as per section 7.1 of the PNI TRAX User Manual. the complete
 * datagram is written to io_ahrs, or until error or EOF.
 *
 * returns number of bytes written, which equals len + 4 if all data was
 * written
 */
size_t ahrs_write(char *data, uint_fast16_t len)
{
	if (!io_ahrs)
	{
		// ahrs_init needs to be called to init io_ahrs
		return 0;
	}
	if (len == 0 || len > 4092) // is it within packet frame size constraints?
	{
		return 0;
	}

	len += 4; // account for 2 length bytes and 2 checksum bytes
	uint_fast16_t crc = crc16_bc(len, data);

	size_t nwrit = write_datagram(len, data, crc);
	fflush(io_ahrs);
	return nwrit;
}
