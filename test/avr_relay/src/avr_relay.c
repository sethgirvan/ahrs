#include <stdio.h>
#include <inttypes.h>

#include "ahrs.h"
#include "io_ahrs.h"
#include "io.h"


int main (int argc, char *argv[])
{
	(void)argc, (void)argv;
	io_init();
	io_stdout_init();

	io_ahrs_init(NULL);
	// May not be strictly necessary, since the data components can be saved to
	// non-volatile memory.
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
	}
	return 0;
}
