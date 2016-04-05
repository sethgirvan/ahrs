#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

#include "ahrs.h"
#include "io.h"
#include "io_ahrs.h"


int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Please pass the name of the file acting as the ahrs on the command line.\n");
		return -1;
	}

	io_init();
	io_stdout_init();
	io_ahrs_init(argv[1]);
	if (!io_ahrs)
	{
		return -1;
	}
	// This seems to help when connecting with the arduino as serial
	// passthrough
	nanosleep(&(struct timespec){.tv_sec = 8}, NULL);

	ahrs_set_datacomp();
	ahrs_cont_start();
	io_ahrs_recv_start(ahrs_att_recv);
	for (;;)
	{
		if (ahrs_att_update())
		{
			printf("P: %f\tR: %f\tY: %f HS: %" PRIuFAST8 "\r", ahrs_att(PITCH),
					ahrs_att(ROLL), ahrs_att(YAW), ahrs_headingstatus());
			fflush(stdout);
		}
		nanosleep(&(struct timespec){.tv_nsec = 1000000L}, NULL);
	}
	return 0;
}
