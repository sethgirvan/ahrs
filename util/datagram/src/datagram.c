/**
 * Purpose: Take input hex data as Trax PNI packet frame, and output a
 * complete datagram containing that packet frame by prepending the proper
 * length bytes and appending the proper checksum bytes.
 *
 * Usage: This program takes hex characters on STDIN. Upon getting a newline
 * or EOF, outputs the datagram of all hex data since the last newline or
 * program start, except for the trailing hex character of an odd number
 * of hex characters, which is ignored. All other characters besides:
 * 0-9a-fA-F and newline are ignored.
 */

#include <stdio.h>
#include <stdint.h>

#include "ahrs_util.h"
#include "dbg.h"
#include "io_ahrs.h"


char buf[4092];

static void check_bufover(size_t *len)
{
	if (*len >= sizeof(buf)/sizeof(buf[0]))
	{
		*len = 0;
		DEBUG("Maximum packet frame length exceeded. Flushing Buffer.");
		ahrs_write(buf, sizeof(buf));
	}
	return;
}


int main(int argc, char *argv[])
{
	(void)argc, (void)argv;
	io_ahrs = stdout;

	int shift = 4;
	for (size_t count = 0;;)
	{
		int c = getchar();

		if ((char)c >= '0' && (char)c <= '9')
		{
			check_bufover(&count);
			buf[count] &= 0x0F << (4 - shift);
			buf[count] |= ((char)c - '0') << shift;
			shift = shift ? 0 : 4;
			if (shift)
			{
				++count;
			}
		}
		else if ((char)c >= 'a' && (char)c <= 'f')
		{
			check_bufover(&count);
			buf[count] &= 0x0F << (4 - shift);
			buf[count] |= ((char)c - 'a' + 10) << shift;
			shift = shift ? 0 : 4;
			if (shift)
			{
				++count;
			}
		}
		else if ((char)c >= 'A' && (char)c <= 'F')
		{
			check_bufover(&count);
			buf[count] &= 0x0F << (4 - shift);
			buf[count] |= ((char)c - 'A' + 10) << shift;
			shift = shift ? 0 : 4;
			if (shift)
			{
				++count;
			}
		}
		else if ((char)c == '\n')
		{
			// on newline, write out current datagram and prepare to
			// create a new datagram
			if (count > 0)
			{
				if (ahrs_write(buf, count) != count + 4)
				{
					return 0;
				}
			}
			count = 0;
		}
		else if (c == EOF)
		{
			if (count > 0)
			{
				if (ahrs_write(buf, count) != count + 4)
				{
					return 0;
				}
			}
			return 0;
		}
	}
}
