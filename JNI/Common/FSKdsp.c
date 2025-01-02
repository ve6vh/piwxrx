/*---------------------------------------------------------------------------

	Project:	      PiWxRx Weather receiver

	Module:		      Signal processing module for ADSP demod

	File Name:		  FSKdsp.c

	Author:		      Martin C. Alcock, VE6VH

	Revision:	      1.05

	Description:	  Contains the dsp modules and threads for the AFSK demodulator.	  
						
				  This program is free software: you can redistribute it and/or modify
				  it under the terms of the GNU General Public License as published by
				  the Free Software Foundation, either version 2 of the License, or
				  (at your option) any later version, provided this copyright notice
				  is included.

				  Copyright (c) 2018-2022 Praebius Communications Inc.

	Revision History:

---------------------------------------------------------------------------*/
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <fcntl.h>
#include <io.h>


#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "rtl.h"

#define	XING_MAX			3					// number of stable samples before a valid zero crossing
#ifdef _WIN32
typedef __ptw32_handle_t pthread_t;
#endif

struct dsp_threads_t {
	int				exit;							// signals an exit
	int				debuglevel;						// current debuglevel

	// these are used by the signal processing thread
	int				wrptr;							// buffer write pointer
	int				rdptr;							// read pointer
	RTL_SAMPLE		buffer[SAMPLE_BFRSIZ];			// data buffer
	pthread_mutex_t	bfr_mutex;						// mutex when writing/reading buffer
	pthread_cond_t	dsp_wait_cond;					// wait condition
	pthread_t		dsp_thread;						// pointer to dsp thread

	// these are the demodulator thread
	BOOL			insync;							// rx is in sync
	pthread_mutex_t	sync_mutex;						// mutex for sync
	void			(*byte_rx_func)(DEMOD_BYTE x);	// pointer to function that processes byte

} dsp_threads;

// bit decpder vars
int divisor;					// outer divisor
int swallow_ctr;				// swallow counter
DEMOD_BYTE current_sense;		// current data sense
BOOL bytesync;					// bytelevel sync
RTL_SAMPLE dcslice_level;		// dc slicer level

// forward refs
static void *dsp_threads_fn(void *arg);

void DSPInit(void (*rx_func)(DEMOD_BYTE x), int debuglevel)
{
	dsp_threads.exit = FALSE;
	dsp_threads.debuglevel = debuglevel;
	dsp_threads.wrptr = 0;
	dsp_threads.rdptr = 0;
	dsp_threads.insync = FALSE;
#ifdef _WIN32
	dsp_threads.bfr_mutex = PTHREAD_MUTEX_INITIALIZER;
	dsp_threads.sync_mutex = PTHREAD_MUTEX_INITIALIZER;
	dsp_threads.dsp_wait_cond = PTHREAD_COND_INITIALIZER;
#endif
	dsp_threads.byte_rx_func = rx_func;
	dsp_threads.insync = FALSE;

	pthread_mutex_init(&dsp_threads.bfr_mutex, NULL);
	pthread_mutex_init(&dsp_threads.sync_mutex, NULL);
	pthread_cond_init(&dsp_threads.dsp_wait_cond, NULL);

	pthread_create(&dsp_threads.dsp_thread, NULL, dsp_threads_fn, (void *)(&dsp_threads));

	// setup signal processing functions
	FIRInit();
	InitOsc();
}

void SetDebugLevel(int debug)
{
	dsp_threads.debuglevel = debug;
}

void DSPDemod(RTL_SAMPLE *PipeBufferPtr, int samples_read)
{
	pthread_mutex_lock(&dsp_threads.bfr_mutex);

	// copy the samples into buffer and decimate to 12 KHz
	int wrptr = dsp_threads.wrptr;
	int sample, samplez1 = 0, sum;
	for (int i = 0; i < samples_read; i++) {
		sum = sample = (int)*PipeBufferPtr++;
		sum += samplez1;
		if (i & 1) {
			dsp_threads.buffer[wrptr] = ((RTL_SAMPLE)(sum >> 1) & 0xffff);
			wrptr = (wrptr + 1) & (SAMPLE_BFRSIZ - 1);
		}
		samplez1 = sample;
	}
	// signal the demod to start
	DEBUGPRINTF("Wrote Samples: signalling DSP threads\n");
	dsp_threads.wrptr = wrptr;
	pthread_cond_signal(&dsp_threads.dsp_wait_cond);
	pthread_mutex_unlock(&dsp_threads.bfr_mutex);

}

void DSPStop(void)
{
	dsp_threads.exit = TRUE;
	pthread_join(dsp_threads.dsp_thread, NULL);
}

void DSPClearSync(void)
{
	pthread_mutex_lock(&dsp_threads.sync_mutex);
	dsp_threads.insync = FALSE;
	pthread_mutex_unlock(&dsp_threads.sync_mutex);
}

static int xingcnt;

BOOL EdgeDetect(DEMOD_BYTE demod_out, BOOL firstTime)
{
	if (firstTime)	{
		current_sense = demod_out;
		xingcnt = 0;
		return FALSE;
	}

	if (demod_out == current_sense) {
		return FALSE;
	}
	
	if (++xingcnt == XING_MAX) {
		current_sense = demod_out;
		xingcnt = 0;
		return TRUE;
	} 
return FALSE;
}

BOOL RunBitClock(BOOL edgedetect)
{

	// if an edge is detected, reset to mid-bit time
	if (edgedetect) {
		divisor = 1 * (BIT_DIVISOR) / 2;		// init to mid-bit time
		swallow_ctr = SWALLOW_CTR;
		return FALSE;
	}

	BOOL bittime = FALSE;
	if (divisor == 0) {
		bittime = TRUE;
		if (swallow_ctr == 0) {
			divisor = BIT_DIVISOR - 1;
			swallow_ctr = SWALLOW_CTR;
		}
		else {
			divisor = BIT_DIVISOR;
			swallow_ctr -= 1;
		}
	}
	else {
		divisor -= 1;
	}
	return bittime;
}

// signal processing thread
static void *dsp_threads_fn(void *arg)
{
	DEMOD_BYTE demod_bit;			// demodulated bit
	DEMOD_BYTE demod_byte;			// demodulated byte
	BOOL bit_time = FALSE;			// at a bit time
	int bitctr;						// bit counter

	struct dsp_threads_t *s = arg;
#ifdef WIN32
	if (s->debuglevel & DEBUG_WRITE)
		_setmode(_fileno(stdout), _O_BINARY);
#endif	
	while (!s->exit) {
		// see if we have any data to process
		pthread_mutex_lock(&s->bfr_mutex);
		if (BUFFER_EMPTY(dsp_threads)) {
			DEBUGPRINTF("MT wait\n");
			pthread_cond_wait(&s->dsp_wait_cond, &s->bfr_mutex);
			DEBUGPRINTF("Got Data\n");
		}
		pthread_mutex_unlock(&s->bfr_mutex);
		// data to process; so process it....
		int rdptr = s->rdptr;
		RTL_SAMPLE sample = s->buffer[rdptr];
		s->rdptr = (rdptr +1)  & (SAMPLE_BFRSIZ - 1);

		if (s->debuglevel & DEBUG_WRITE) {
			fwrite(&sample, sizeof(RTL_SAMPLE), 1, stdout);
		}
		else {
			// run the oscillator first
			RTL_SAMPLE Iosc = RunOsc(I_CHANNEL);
			RTL_SAMPLE Qosc = RunOsc(Q_CHANNEL);
			BACKGDEBUG(DEBUG_OSC)
				fprintf(stderr, "%04x %f\n", sample & 0xffff, ((double)sample / 32767.0));


			// run the mixer
			int Imix = (RTL_SAMPLE)(((int)Iosc*(int)sample) >> 15);
			int Qmix = (RTL_SAMPLE)(((int)Qosc*(int)sample) >> 15);

			// low pass filter the samples
			int Iout = RunFIR(Imix, I_CHANNEL);
			int Qout = RunFIR(Qmix, Q_CHANNEL);
			BACKGDEBUG(DEBUG_LPF)
				fprintf(stderr, "%04x %04x\n", Iout & 0xffff, Qout & 0xffff);

			// now run the demodulator
			int phase = PhaseDiscrim(Iout, Qout);
			dcslice_level = (int)(dcslice_level*0.99985) + (int)(phase*.00015);
			phase -= dcslice_level;

			demod_bit = (phase > 0) ? 0 : 1;
			BACKGDEBUG(DEBUG_DEMOD)
				fprintf(stderr, "%f, %f, %d\n",  ((double)phase / 32767.0), ((double)dcslice_level / 32767.0), bit_time);

			// not in sync yet?
			if (!s->insync) {
				bytesync = FALSE;

				pthread_mutex_lock(&s->sync_mutex);
				s->insync = SyncCorrelator(demod_bit);
				BACKGDEBUG(DEBUG_SYNC)
					fprintf(stderr, "DSP SYNC achieved\n");

				if (s->insync) {
					// initialize edge detector and bit clock
					EdgeDetect(demod_bit, TRUE);
					RunBitClock(TRUE);
					bitctr = 0;
					demod_byte = 0;
					BACKGDEBUG(DEBUG_SYNC)
						fprintf(stderr,"DSP SYNC achieved\n");
				}
				pthread_mutex_unlock(&s->sync_mutex);

			}
			// we are in sync; gather the bits up
			else {
				bit_time = RunBitClock(EdgeDetect(demod_bit, FALSE));

				BACKGDEBUG(DEBUG_BITSHIFT)
					fprintf(stderr, "%d %d\n", bit_time, demod_bit);

				if (bit_time) {
					BACKGDEBUG(DEBUG_BYTEOUT) {
						if (bitctr == BITSPERBYTE - 1)
							fprintf(stderr, "%d\n", demod_bit);
						else
							fprintf(stderr, "%d,", demod_bit);
					}

					// receive the byte and sync to the data
					demod_byte = (demod_byte >> 1) | ((demod_bit & 1) << 7);
					if (!bytesync) {
						if (demod_byte == SYNC_BYTE) {
							bytesync = TRUE;
							bitctr = 0;
							(*s->byte_rx_func)(demod_byte);
						}
					}
					else {
						if (bitctr == BITSPERBYTE - 1) {
							(*s->byte_rx_func)(demod_byte);
							bitctr = 0;
						}
						else bitctr++;
					}
				}
			}
		} 
	}
	return NULL;
}
