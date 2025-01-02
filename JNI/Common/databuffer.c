/*---------------------------------------------------------------------------
	Project:	      PiWxRx Weather receiver

	Module:		      Ex data buffering

	File Name:	      databuffer.c

	Author:		      Martin C. Alcock, VE6VH

	Revision:	      1.05

	Description:	Code for buffering data up to the application layer

					This program is free software: you can redistribute it and/or modify
					it under the terms of the GNU General Public License as published by
					the Free Software Foundation, either version 2 of the License, or
					(at your option) any later version, provided this copyright notice
					is included.

				  Copyright (c) 2018-2022 Praebius Communications Inc.

	Revision History:

---------------------------------------------------------------------------*/

#include <pthread.h>
#include <stdio.h>

#include "rtl.h"

#define	bufferSIZE		128

// data buffer struct
struct data_buffer_t {
	int				wrptr;						// buffer write pointer
	int				rdptr;						// read pointer
	DEMOD_BYTE		buffer[bufferSIZE];		// data buffer
	pthread_mutex_t	data_mutex;					// data mutex
	pthread_cond_t	data_wait_cond;				// data wait condition
} data_buffer;

void databuffer_init(void) {

	// initialize the buffer to NO_DATA_RX in case of a pointer runaway
	data_buffer.wrptr = data_buffer.rdptr = 0;
	for (int i = 0; i < bufferSIZE; i++)
		data_buffer.buffer[i] = NO_BYTE;;

	// initialize data wait condition
#ifdef _WIN32
	data_buffer.data_mutex = PTHREAD_MUTEX_INITIALIZER;
	data_buffer.data_wait_cond = PTHREAD_COND_INITIALIZER;
#endif
	pthread_mutex_init(&data_buffer.data_mutex, NULL);
	pthread_cond_init(&data_buffer.data_wait_cond, NULL);
}

// callback for byte received from FSK decoder
void databuffer_put(DEMOD_BYTE byterx)
{
	pthread_mutex_lock(&data_buffer.data_mutex);

	data_buffer.buffer[data_buffer.wrptr] = byterx;
	data_buffer.wrptr = (data_buffer.wrptr + 1) & (bufferSIZE - 1);

	pthread_cond_signal(&data_buffer.data_wait_cond);
	pthread_mutex_unlock(&data_buffer.data_mutex);

	DEBUGLEVEL(DEBUG_JNI)
		fprintf(stderr, "wrote byte\n");
}

DEMOD_BYTE databuffer_get(void)
{
	// take them out here...
	pthread_mutex_lock(&data_buffer.data_mutex);
	if (BUFFER_EMPTY(data_buffer)) {
		DEBUGLEVEL(DEBUG_JNI)
			fprintf(stderr, "No bytes: waiting\n");
		pthread_cond_wait(&data_buffer.data_wait_cond, &data_buffer.data_mutex);
		DEBUGPRINTF("Got Data\n");
	}
	pthread_mutex_unlock(&data_buffer.data_mutex);

	char retval = data_buffer.buffer[data_buffer.rdptr];
	data_buffer.rdptr = (data_buffer.rdptr + 1) & (bufferSIZE - 1);
	DEBUGLEVEL(DEBUG_JNI)
		fprintf(stderr, "read byte\n");
	return retval;
}
