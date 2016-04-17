/**
 * Description: Handles communication with the PNI TRAX above the level of
 * 'FILE *' streams.
 *
 * The PNI TRAX user manual was used as a reference for this file, and an
 * effort was made to use the terminology used in that document.
 */

#include <assert.h>
#include <string.h>

#ifndef IEEE754
#include <math.h> // to deserialize floats from trax
#endif

#include <stdint.h>
#include <stdio.h>

#include "ahrs.h"
#include "io_ahrs.h"
#include "crc_xmodem.h"
#include "dbg.h"
#include "macrodef.h"


#define BAUD 38400UL

// TODO: change this to defines

// 2 Byte Count + 1 Frame Id + 1 ID Count + 3 * (1 Component ID + 4 float32) +
// 2 CRC
static uint_fast16_t const DATAGRAM_BYTECOUNT = 23U;
static unsigned char const FRAME_ID = 0x05U; // kGetDataResp command
// 1 Heading + 1 Pitch + 1 Roll + 1 HeadingStatus (in any order in payload)
static uint_fast8_t const ID_COUNT = 4U;

static uint16_t const CRC_POST_ID_COUNT = 0x7982U; // crc of first 4 assumed byte values


float const ahrs_range[NUM_ATT_AXES][2] = {
	[PITCH] = {[COMPONENT_MIN] = -90.f, [COMPONENT_MAX] = 90.f},
	[YAW] = {[COMPONENT_MIN] = 0.f, [COMPONENT_MAX] = 360.f /* Should technically be the next lower float */},
	[ROLL] = {[COMPONENT_MIN] = -180.f, [COMPONENT_MAX] = 180.f}};

// triple buffer coordinated with io_ahrs_tripbuf... functions
static struct ahrs
{
	float att[NUM_ATT_AXES];
	uint_fast8_t headingstatus;
} ahrs[3];


float ahrs_att(enum att_axis const dir)
{
	return ahrs[io_ahrs_tripbuf_read()].att[dir];
}

uint_fast8_t ahrs_headingstatus()
{
	return ahrs[io_ahrs_tripbuf_read()].headingstatus;
}

bool ahrs_att_update()
{
	return io_ahrs_tripbuf_update();
}

static enum
{
	INIT,
	SYNC,
	COMPONENT_ID,
#ifdef IEEE754
	ANGLE,
#else
	ANGLE_BYTE0,
	ANGLE_BYTE1,
	ANGLE_BYTE2,
	ANGLE_BYTE3,
#endif
	HEADINGSTATUS,
	CRC1,
	CRC2
} state = INIT;

/*
 * Stateful coroutine style parsing. I felt stack switching wasn't worth it for
 * this one case. Macros weren't used for the 'state = foo; return; case foo:'
 * blocks because I felt it didn't increase clarity.
 *
 * returns true when a valid attitude data set has just been completely parsed.
 */
static bool parse_att(unsigned char const c)
{
	switch (state)
	{
		case INIT:;
		{
			/* first four bytes of datagram:
			 *
			 * sync[(idx + 1) % 4] == Most significant bytecount byte
			 * sync[(idx + 2) % 4] == Least significant bytecount byte
			 * sync[(idx + 3) % 4] == Frame ID
			 * sync[idx] == Data Components ID Count
			 *
			 * These are fixed for every datagram we can parse. ie a
			 * 'kGetDataResp' packet containing only the three data components
			 * 'kHeading', 'kPitch', and 'kRoll', in any order. Therefore, they
			 * are used to try to synchronize with a datagram in the case the
			 * expected values are not recieved. Even if sychronization is
			 * handled elsewhere (eg based on timing), this seems like  a
			 * decently logical way to handle unexpected values.
			 */
			static unsigned char sync[4];
			memset(sync, 0xFF, 4); // placeholder val equal to no expected val
			static uint_fast8_t idx = 3;
			while ((sync[idx] = c) != ID_COUNT ||
					sync[(idx + 3) % 4] != FRAME_ID ||
					sync[(idx + 2) % 4] != (DATAGRAM_BYTECOUNT & 0x00FF) ||
					sync[(idx + 1) % 4] != DATAGRAM_BYTECOUNT >> 8)
			{
				// This will always loop at least thrice as sync is populated.

				// First four bytes not what expected. Resynchronize assuming
				// the byte just received was the Frame ID.
				idx = (idx + 1) % 4;

				state = SYNC;
				return false;
		case SYNC:;
			}
		}

		static uint16_t crc;
		// no need to compute the crc of the first four bytes at runtime, since
		// their values are already assumed
		crc = CRC_POST_ID_COUNT;

		// Just get the triple buffer write index once, since it can't
		// change until io_ahrs_tripbuf_offer is invoked.
		static unsigned char write_idx;
		write_idx = io_ahrs_tripbuf_write();

		static struct cir
		{
			unsigned char att           : 3; // Bit positions PITCH, YAW, ROLL
			unsigned char headingstatus : 1;
		} comp_is_read;
		comp_is_read = (struct cir){0, 0};
		static uint_fast8_t i;
		for (i = ID_COUNT; i--; crc = crc_xmodem_update(crc, c))
		{
			state = COMPONENT_ID;
			return false;
		case COMPONENT_ID:;

			static uint_fast8_t dir;
			// data components may arrive in arbitrary order
			if (c == 5) // kHeading component
			{
				dir = YAW;
			}
			else if (c == 24) // kPitch component
			{
				dir = PITCH;
			}
			else if (c == 25) // kRoll component
			{
				dir = ROLL;
			}
			else if (c == 79) // kHeadingStatus component
			{
				if (comp_is_read.headingstatus)
				{
					// repeat component, fail datagram
					DEBUG("Repead component.");
					state = INIT;
					return false;
				}
				comp_is_read.headingstatus = true;

				crc = crc_xmodem_update(crc, c);

				state = HEADINGSTATUS;
				return false;
		case HEADINGSTATUS:;

				ahrs[write_idx].headingstatus = c;
				continue;
			}
			else // unrecognized component type
			{
				// fail datagram
				DEBUG("Unrecognized component.");
				state = INIT;
				return false;
			}

			if (comp_is_read.att & (1U << dir))
			{
				// component is a repeat, fail datagram
				DEBUG("Repeat component.");
				state = INIT;
				return false;
			}

/* There doesn't seem to be any compiler-defined macros to check for IEEE754
 * format floats. GCC never defines __STD_IEC_559__, since it doesn't conform.
 * avr-gcc uses IEEE754 format little endian floats.
 */
#ifdef IEEE754
			/* The native 'float' must be stored in single precision IEEE754
			 * format.
			 */
#ifndef AVR // avr-libc doesn't seem to support static_assert
			static_assert(sizeof(float) == 4, "float must not be IEEE754. Try compiling without 'IEEE754' defined.");
#endif
			{
				static uint_fast8_t j;
				for (j = 4; j--;)
				{
					crc = crc_xmodem_update(crc, c);
					state = ANGLE;
					return false;
		case ANGLE:;
					// The ahrs transmits floats as big endian by default,
					// while native floats are assumed to be little endian.
					// 
					// type-pun float in order to write raw bytes
					((unsigned char *)&ahrs[write_idx].att[dir])[j] = c;
				}
			}
#else // translate floating point data to native format portably

			// FIXME: This code doesn't parse the floats correctly
			/* The ahrs transmits a single precision IEEE754 float, big endian
			 * by default.
			 *
			 *       sign  expon       mantissa
			 * bit:     01   -  89          -         31
			 * byte:    [  0   ][  1   ][  2   ][  3   ]
			 */
			{
				crc = crc_xmodem_update(crc, c);
				state = ANGLE_BYTE0;
				return false;
		case ANGLE_BYTE0:;

				static bool sign; // 1 bit sign
				sign = c >> 7;

				static uint8_t expon; // 8 bit expon
				expon = 0;
				expon |= (uint8_t)c << 1;

				crc = crc_xmodem_update(crc, c);
				state = ANGLE_BYTE1;
				return false;
		case ANGLE_BYTE1:;

				expon |= (uint8_t)c >> 7;
				if (expon == 0xFFU)
				{
					DEBUG("Infinity or NaN received.");
					// float must be +\- infinity or NaN, fail datagram
					state = INIT;
					return false;
				}

				static uint_fast32_t mantissa; // 23 bit mantissa
				mantissa = 0;
				// mask c to avoid writing a 1 to bit 24
				mantissa |= ((uint_fast32_t)c & 0x7FU) << 16;

				crc = crc_xmodem_update(crc, c);
				state = ANGLE_BYTE2;
				return false;
		case ANGLE_BYTE2:;

				mantissa |= (uint_fast32_t)c << 8;

				crc = crc_xmodem_update(crc, c);
				state = ANGLE_BYTE3;
				return false;
		case ANGLE_BYTE3:;

				mantissa |= c;
				// All the float data has been received
				if (expon == 0x00U)
				{
					if (mantissa != 0)
					{
						DEBUG("Subnormal number received.");
						// subnormal number, fail datagram
						state = INIT;
						return false;
					}
					// float is +/- 0
					ahrs[write_idx].att[dir] = 0.f;
				}
				else
				{
					// float is a normalized value
					ahrs[write_idx].att[dir] = mantissa;
					// The significand is 1.mantissa, so the exponent needs to
					// be decreased by the number of mantissa bits. expon also
					// has a bias (0 offset) of 127.
					int power = expon - 23 - 127;
					ahrs[write_idx].att[dir] *= exp2f(power);
					if (sign)
					{
						ahrs[write_idx].att[dir] *= -1;
					}
				}
			}
#endif
			/* TODO: Check and scale angle values
			 * They come from the ahrs in this range:
			 * heading: 0.0 - 359.9
			 * pitch: -90 - 90
			 * roll: -180 - 180

			// kHeading, kPitch, and kRoll, all give a value in a 4 byte/32
			// bit format, in degrees, in the range [0, 360).
			if (ahrs[write_idx].att[dir] < 0.f ||
					360.f <= ahrs[write_idx].att[dir])
			{
				// received attitude value outside of the expected range
				state = INIT;
				return false;
			}
			ahrs[write_idx].att[dir] \= 360.f; // scale to be from 0.0 - 1.0
			*/

			comp_is_read.att |= 1U << dir; // valid value has been read for this dir
		}
		if (!~comp_is_read.att || !comp_is_read.headingstatus)
		{
			DEBUG("Not all data components received.");
			// fail datagram
			state = INIT;
			return false;
		}

		state = CRC1;
		return false;
		case CRC1:;

		// last two bytes are the crc appended to the data packet
		crc = crc_xmodem_update(crc, c);

		state = CRC2;
		return false;
		case CRC2:;

		// The crc of a value with its crc appended == 0, so the crc is valid
		// if the crc of the entire datagram == 0.
		if (crc_xmodem_update(crc, c) == 0x0000)
		{
			io_ahrs_tripbuf_offer();
			// Datagram and all attitude data is considered valid
			state = INIT;
			return true;
		}
		else
		{
			// Invalid crc, attitude data will be discarded
			DEBUG("Invalid CRC: %04X", crc_xmodem_update(crc, c));
			state = INIT;
			return false;
		}
	}
	assert(0 /* Should never be reached. */);
}

/*
 * Sets state to be equivalent to what it was the first time parse_att()
 * was run.
 *
 * parse_att() will consider the next byte passed to potentially
 * be the first byte of a datagram.
 */
void ahrs_parse_att_reset()
{
	state = INIT;
	return;
}

int ahrs_att_recv()
{
	int c;
	if ((c = getc(io_ahrs)) == EOF)
	{
		return EOF;
	}
	return parse_att(c);
}

/**
 * writes n bytes from data to io_ahrs, or until error or EOF
 *
 * returns number of bytes written, which equals n if all data was written
 */
static size_t ahrs_write_raw(void const * const datagram, size_t const n)
{
	assert(io_ahrs /* ahrs_io has not been successfully initialized */);

	size_t const nwrit = fwrite(datagram, 1, n, io_ahrs);
	fflush(io_ahrs); // this is probably a null operation on avr
	return nwrit;
}

int ahrs_set_datacomp()
{
	/* Data components must be set at least each time the ahrs is powered. At
	 * least AFAIK, there isn't a way to set this persistently.
	 *
	 * Datagram to set data components to:
	 * kHeading, kPitch, kRoll
	 * parse_att() allows them to be in any order
	 */
	static unsigned char const datagram_set_comp[] = {
			0x00, 0x0A, // bytecount
			0x03, // Frame ID: kSetDataComponents
			0x04, // ID Count
			// Component IDs: kHeading, kPitch, kRoll, kHeadingStatus respectively
			5, 24, 25, 79,
			0xE2, 0xEF}; // crc
	if (ahrs_write_raw(datagram_set_comp, sizeof(datagram_set_comp)) !=
			sizeof(datagram_set_comp))
	{
		DEBUG("Failed sending kSetDataComponents command.");
		return -1;
	}
	return 0;
}

int ahrs_cont_start()
{
	static unsigned char const datagram_start_cont[] = {
			0x00, 0x05, // bytecount
			0x15, // Frame ID: kStartContinuousMode
			0xBD, 0x61}; // crc

	if (ahrs_write_raw(datagram_start_cont, sizeof(datagram_start_cont)) !=
			sizeof(datagram_start_cont))
	{
		DEBUG("Failed sending kStartContinuousMode command.");
		return -1;
	}
	return 0;
}
