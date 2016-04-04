#ifndef AHRS_H
#define AHRS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>


enum att_axis {PITCH, YAW, ROLL, NUM_ATT_AXES};


/**
 * Tells ahrs to start sending data in continous mode.
 *
 * Make sure the desired data components are set first, such as with
 * ahrs_set_datacomp().
 *
 * returns 0 on success
 */
int ahrs_cont_start();

/**
 * The values returned will not changed until ahrs_att_update() is called and it
 * returns true.
 */
float ahrs_att(enum att_axis dir);

/**
 * Updates the values returned by ahrs_att to the newest complete set
 * of data that has been received from the ahrs before some point in time
 * within this function's lifetime.
 *
 * Should only be called between complete "uses" of the attitude data to
 * avoid using disparate data together (ie between runs of a PID routine).
 *
 * Returns true when there has been a new complete set of data received from
 * the ahrs since the last time ahrs_att_update() has been called.
 */
bool ahrs_att_update();

/*
 * 
 *
 * returns true if at least one valid data set has been completely read from
 * the ahrs.
 */
int ahrs_att_recv();

/**
 * Causes any data from a current incomplete datagram to be discarded. The next
 * received byte will be treated as potentially the start of a datagram.
 *
 * Can be used due for, eg, a serial timeout for intra-datagram data.
 */
void parse_att_reset();

int ahrs_set_datacomp();

#ifdef __cplusplus
}
#endif

#endif
