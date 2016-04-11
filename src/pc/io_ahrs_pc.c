#define _POSIX_C_SOURCE 1
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>

#include "io_ahrs.h"
#include "macrodef.h"


FILE *io_ahrs;

static pthread_t thread_recv;

static int (*handler_recv)();


void io_ahrs_init(char const *path)
{
	io_ahrs = fopen(path, "r+");
	// TODO: correctly handle termios
	return;
}

void io_ahrs_clean()
{
	fclose(io_ahrs);
	return;
}

static void *ahrs_recv_thread(void *arg)
{
	(void)arg;
	for (;;)
	{
		if (handler_recv() == EOF && feof(io_ahrs))
		{
			return NULL;
		}
	}
}

int io_ahrs_recv_start(int (*handler)())
{
	handler_recv = handler;
	return pthread_create(&thread_recv, NULL, ahrs_recv_thread, NULL);
}

void io_ahrs_recv_stop()
{
	pthread_cancel(thread_recv);
	return;
}

static struct
{
	unsigned char write : 2;
	unsigned char clean : 2;
	unsigned char read  : 2;
	unsigned char new   : 1;
	pthread_mutex_t lock;
} tripbuf = {0, 1, 2, false, PTHREAD_MUTEX_INITIALIZER};

bool io_ahrs_tripbuf_update()
{
	pthread_mutex_lock(&tripbuf.lock);

	assert(IN_RANGE(0, tripbuf.write, 2) && IN_RANGE(0, tripbuf.clean, 2) &&
			IN_RANGE(0, tripbuf.read, 2) && IN_RANGE (0, tripbuf.new, 1));

	bool updated;
	if (tripbuf.new)
	{
		tripbuf.new = false;
		unsigned char tmp = tripbuf.read;
		tripbuf.read = tripbuf.clean;
		tripbuf.clean = tmp;
		updated = true;
	}
	else
	{
		updated = false;
	}
	pthread_mutex_unlock(&tripbuf.lock);
	return updated;
}

void io_ahrs_tripbuf_offer()
{
	pthread_mutex_lock(&tripbuf.lock);

	assert(IN_RANGE(0, tripbuf.write, 2) && IN_RANGE(0, tripbuf.clean, 2) &&
			IN_RANGE(0, tripbuf.read, 2) && IN_RANGE (0, tripbuf.new, 1));

	unsigned char tmp = tripbuf.write;
	tripbuf.write = tripbuf.clean;
	tripbuf.clean = tmp;
	tripbuf.new = true;

	pthread_mutex_unlock(&tripbuf.lock);
}

unsigned char io_ahrs_tripbuf_write()
{
	return tripbuf.write;
}

unsigned char io_ahrs_tripbuf_read()
{
	return tripbuf.read;
}
