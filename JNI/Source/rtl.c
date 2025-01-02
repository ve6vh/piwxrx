/*---------------------------------------------------------------------------

	Project:	      PiWxRx Weather receiver

	Module:		      Real time code for test main

	File Name:		  rtl.c

	Author:		      Martin C. Alcock

	Revision:	      1.05

	Description:	  Manages the interface between the Java code and the RTL dongle. Processes
                  audio samples into two streams, one for UDP and the other for the FSK
                  demodulator.
                  
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

#else
#include <signal.h>
#include <time.h>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "rtl.h"

//header feilds
#define	HDR1		    0		// first header byte
#define	HDR2		    1		// Second header
#define	SSEQ		    2		// sequence ID
#define	TIMESTAMP	    4		// timestamp
#define	SYNCSRC		    8		// sync src ID

#define	MARK		    0x80	// mark bit

// struct to manage timed thread
struct timer_threads_t {
	BOOL			exit;
	RTL_SAMPLE		*PipeBufferPtr;
	BOOL			SendingUDP;
	int			debuglevel;
	pthread_t		timer_fn;
	pthread_mutex_t	timer_mutex;				// timer mutex
	pthread_cond_t	timer_wait_cond;			// timer wait condition
	pthread_mutex_t	timer_exit_mutex;			// timer exit mutex
	pthread_cond_t	timer_exit_wait_cond;		// timer exit wait condition    
} timer_threads;

static void *timer_threads_fn(void *arg);

#ifndef __DEBUGLEVEL
#define	__DEBUGLEVEL
int 	debuglevel;
#endif

/*--------------------------------------------------------------------------

	FUNCTION:	InitRTL

	INPUTS:		command line

	OUTPUTS:	TRUE or FALSE

	DESCRIPTION:	start the child process

---------------------------------------------------------------------------*/
BOOL InitRTL(char *cmdline, void (*rx_func)(DEMOD_BYTE x), int debug)
{

	debuglevel = debug;

	// start the child process
	if (!initChildProcess((char *)cmdline)) {
		DEBUGPRINTF("Could not start child process\n");
		return(FALSE);
	}
	DEBUGPRINTF("Child process started successfully\n");

	DSPInit(rx_func, debuglevel);

	timer_threads.SendingUDP = FALSE;
	timer_threads.exit = FALSE;
#ifdef _WIN32
	timer_threads.timer_mutex = PTHREAD_MUTEX_INITIALIZER;
	timer_threads.timer_wait_cond = PTHREAD_COND_INITIALIZER;
	timer_threads.timer_exit_mutex = PTHREAD_MUTEX_INITIALIZER;
	timer_threads.timer_exit_wait_cond = PTHREAD_COND_INITIALIZER;    
#endif
	timer_threads.debuglevel = debug;

	pthread_mutex_init(&timer_threads.timer_mutex, NULL);
	pthread_cond_init(&timer_threads.timer_wait_cond, NULL);
	pthread_mutex_init(&timer_threads.timer_exit_mutex, NULL);
	pthread_cond_init(&timer_threads.timer_exit_wait_cond, NULL);    
	
	if(debuglevel & DEBUG_DEEMPHASIS)
		fprintf(stderr, "Applying deemphasis to Codec\n");
	 
	return(TRUE);
}

/*---------------------------------------------------------------------------

	FUNCTION:	    RunRTL (Windows version)

	INPUTS:		    none

	OUTPUTS:	    TRUE or FALSE

	DESCRIPTION:	start the background thread and timer. Invoke the thread
                    to read the pipe from the background process every 30 ms

---------------------------------------------------------------------------*/
#ifdef _WIN32
BOOL RunRTL(void)
{
	// alloc buffers...
	if ((timer_threads.PipeBufferPtr = (RTL_SAMPLE *)malloc((size_t)(sizeof(RTL_SAMPLE) * (PIPE_READ_LEN + SPARE)))) == NULL) {
		DEBUGPRINTF("Memory Allocation Error\n");
		return(FALSE);
	}
	DEBUGPRINTF("Alloc passed\n");


	HANDLE hTimer;
	// windows timer code...
	if ((hTimer = CreateWaitableTimer(NULL, TRUE, NULL)) == NULL) {
		DEBUGPRINTF("Create Timer failed\n");
		free(timer_threads.PipeBufferPtr);
		return(FALSE);
	}
	DEBUGPRINTF("Timer created\n");

	// start the background thread
	pthread_create(&timer_threads.timer_fn, NULL, timer_threads_fn, (void *)(&timer_threads));

	do {
		SetWaitableTimer(hTimer, 0LL, TIMER_VALUE, NULL, NULL, FALSE);
		WaitForSingleObject(hTimer, TIMER_VALUE);
		pthread_mutex_lock(&timer_threads.timer_mutex);
		pthread_cond_signal(&timer_threads.timer_wait_cond);
		pthread_mutex_unlock(&timer_threads.timer_mutex);
	} while (!timer_threads.exit);

	CancelWaitableTimer(hTimer);
    
    // stop the background thread
    pthread_join(timer_threads.timer_fn, NULL);

	free(timer_threads.PipeBufferPtr);
	DEBUGLEVEL(DEBUG_MSGS)
		fprintf(stderr, "RTL process stopped\n");
	return TRUE;
}
/*---------------------------------------------------------------------------

	FUNCTION:	    RunRTL (Linux version)

	INPUTS:		    none

	OUTPUTS:	    TRUE or FALSE

	DESCRIPTION:	start the background thread and timer. Invoke the thread
                    to read the pipe from the background process every 30 ms

---------------------------------------------------------------------------*/
#else
// signal handler    
static void
RTL_timer_signal(int sig, siginfo_t *si, void *uc)
{
    /* Not a timer signal? */
    if (!si || si->si_code != SI_TIMER)	{
        return;
    }
    
	pthread_mutex_lock(&timer_threads.timer_mutex);
	pthread_cond_signal(&timer_threads.timer_wait_cond);
	pthread_mutex_unlock(&timer_threads.timer_mutex);        
}
    
BOOL RunRTL(void)
{
    struct sigaction sa;
    struct sigevent sev;
    sigset_t mask;
    long long freq_nanosecs;
    struct itimerspec its;    
    timer_t timerid;

	// alloc buffers...
	if ((timer_threads.PipeBufferPtr = (RTL_SAMPLE *)malloc((size_t)(sizeof(RTL_SAMPLE) * (PIPE_READ_SIZE + SPARE)))) == NULL) {
		DEBUGPRINTF("Memory Allocation Error\n");
		return(FALSE);
	}
	DEBUGPRINTF("Alloc passed\n");

    // step 1: establish a handle for timer signal
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = RTL_timer_signal;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIG, &sa, NULL) == -1)    {
    	DEBUGPRINTF("sigaction Error\n");
		return(FALSE);
	}

    // step 2: block it temporarily
    sigemptyset(&mask);
    sigaddset(&mask, SIG);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)    {
    	DEBUGPRINTF("sigprocmask Error\n");
		return(FALSE);
	}        

    // step 3: create the timer
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCKID, &sev, &timerid) == -1)    {
        DEBUGPRINTF("timer_create Error\n");
		return(FALSE);
	}   

	// start the background thread
	pthread_create(&timer_threads.timer_fn, NULL, timer_threads_fn, (void *)(&timer_threads));
    
    // now start the timer
    freq_nanosecs = MSTONS(TIMER_VALUE);
    its.it_value.tv_sec = freq_nanosecs / 1000000000;
    its.it_value.tv_nsec = freq_nanosecs % 1000000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    if (timer_settime(timerid, 0, &its, NULL) == -1)    {
        DEBUGPRINTF("timer_settime Error\n");
		return(FALSE);
    }

    // unblock the signal
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)    {
        DEBUGPRINTF("sigprocmask Error\n");
		return(FALSE);
    }    

    // wait for the signal to stop
	pthread_mutex_lock(&timer_threads.timer_exit_mutex);
    while(!timer_threads.exit)  {
        pthread_cond_wait(&timer_threads.timer_exit_wait_cond, &timer_threads.timer_exit_mutex);
    }
    pthread_mutex_unlock(&timer_threads.timer_exit_mutex);   
    
    // stop the timer
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    timer_settime(timerid, 0, &its, NULL);
    
    // stop the background thread
    pthread_join(timer_threads.timer_fn, NULL);

	free(timer_threads.PipeBufferPtr);
	DEBUGLEVEL(DEBUG_MSGS)
		fprintf(stderr, "Timer process stopped\n");
	return TRUE;
}    
#endif

/*---------------------------------------------------------------------------

	FUNCTION:	timer_threads_fn

	INPUTS:		timer thread struct

	OUTPUTS:	none

	DESCRIPTION:	read the pipe every 30 ms and dispatch the other processes

---------------------------------------------------------------------------*/
static void *timer_threads_fn(void *arg)
{
	int samples_read = 0;
	struct timer_threads_t *s = arg;

    // read the pipe every 30 ms
	while (!s->exit) {
		pthread_mutex_lock(&s->timer_mutex);
		pthread_cond_wait(&s->timer_wait_cond, &s->timer_mutex);
		pthread_mutex_unlock(&s->timer_mutex);
		samples_read = ReadFromPipe(s->PipeBufferPtr, PIPE_READ_SIZE);
		if (samples_read > 0) {
			int newsamples = PipeDecimate(s->PipeBufferPtr, samples_read, PIPE_READ_LEN);
			if (s->SendingUDP) {
				SendUDPPacket(s->PipeBufferPtr, newsamples);
				DEBUGPRINTF("Packet Sent\n");
			} else {
				DEBUGLEVEL(DEBUG_MSGS)
					fprintf(stderr, "Read %d samples: %x\n", newsamples, debuglevel);
			}
			DSPDemod(s->PipeBufferPtr, newsamples);
		} 
    }
	return NULL;
}

/*---------------------------------------------------------------------------

	FUNCTION:	StopRTL

	INPUTS:		none

	OUTPUTS:	none

	DESCRIPTION:	signal the rtl process to stop

---------------------------------------------------------------------------*/
void StopRTL(void)
{
	timer_threads.exit = TRUE;
	DSPStop();
	CloseChildProcess();
    
#ifndef __WIN32
// linux process is waiting on a signal
	pthread_mutex_lock(&timer_threads.timer_exit_mutex);
	pthread_cond_signal(&timer_threads.timer_exit_wait_cond);
	pthread_mutex_unlock(&timer_threads.timer_exit_mutex);  
#endif    
}

/*---------------------------------------------------------------------------

	FUNCTION:	ClrFSKSync

	INPUTS:		none

	OUTPUTS:	none

	DESCRIPTION:	clear a sync condition in the FSK receiver

---------------------------------------------------------------------------*/
void ClrFSKSync(void)
{
	DSPClearSync();
}

/*---------------------------------------------------------------------------

	FUNCTION:	StartUDP

	INPUTS:		RTL header, remote ip/port, myip/port

	OUTPUTS:	TRUE or FALSE

	DESCRIPTION:	start sending UDP packets

---------------------------------------------------------------------------*/
BOOL StartUDP(unsigned char *hdrPtr, int nhdrbytes, char *remoteip, int remotePort, char *myip, int myport, int codec, int gain)
{

    if(!InitUDP(hdrPtr, nhdrbytes, remoteip, remotePort, myip, myport, codec, gain))    {
        DEBUGPRINTF("Start UDP failed\n");
        return FALSE;
    }

	DEBUGPRINTF("UDP Started\n");
    timer_threads.SendingUDP = TRUE;

	return TRUE;
}

/*---------------------------------------------------------------------------

	FUNCTION:	StopUDP

	INPUTS:		none

	OUTPUTS:	none

	DESCRIPTION:	signal the rtl process to stop

---------------------------------------------------------------------------*/
void StopUDP(void)
{	
	timer_threads.SendingUDP = FALSE;
    CloseUDP();
}


