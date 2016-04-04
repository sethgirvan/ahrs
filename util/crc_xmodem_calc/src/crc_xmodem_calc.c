#include <stdio.h>

#include "crc_xmodem.h"


int main(int argc, char *argv[])
{
	(void)argc, (void)argv;
	uint16_t crc = CRC_XMODEM_INIT_VAL;
	for (int c; (c = getchar()) != EOF;)
	{
		crc = crc_xmodem_update(crc, c);
	}
	printf("crc:\n");
	printf("decimal: %u\n", crc);
	printf("hex: %x\n", crc);
}
