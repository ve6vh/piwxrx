/*---------------------------------------------------------------------------
	Project:	      PiWxRx Weather receiver

	Module:		      Main RTL JNI layer code

	File Name:		  fir.c

	Author:		      Martin C. Alcock

	Revision:	      1.05

	Description:	  This module implements an FIR filter to extract the modulated carrier
                      post-mixing.
                  
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
#endif

#include <stdio.h>
#include <stdlib.h>

#include "rtl.h"

// life changing parameters
#define USE_HANN_LP		1					// use hann lp instead of kaiser
#define	FIR_DEBUG		0					// debug mode

#if USE_HANN_LP
#define N_FIR_TAPS      45					// number of taps
#define MAC_SHIFT		16					// shift out of MAC
#else
#define N_FIR_TAPS      17					// number of taps
#define MAC_SHIFT		15					// shift out of MAC
#endif

// FIR data struct
typedef struct fir_data {
	int chnum;
	int wrptr;
	int rdptr;
	int firdata[N_FIR_TAPS];
} FIR_DATA;

#if USE_HANN_LP
// Hann Window low pass filter
// Roll off at 300 Hz, -60 dB @ 1300 Hz
FIR_COEFF Fir_Coeffs[N_FIR_TAPS] = {
	/*  0 */ 0x0018,
	/*  1 */ 0x0055,
	/*  2 */ 0x0097,
	/*  3 */ 0x00D0,
	/*  4 */ 0x00EC,
	/*  5 */ 0x00D9,
	/*  6 */ 0x008A,
	/*  7 */ 0x0000,
	/*  8 */ 0xFF45,
	/*  9 */ 0xFE75,
	/*  A */ 0xFDB7,
	/*  B */ 0xFD3D,
	/*  C */ 0xFD37,
	/*  D */ 0xFDCD,
	/*  E */ 0xFF17,
	/*  F */ 0x0114,
	/* 10 */ 0x03AC,
	/* 11 */ 0x06AA,
	/* 12 */ 0x09C4,
	/* 13 */ 0x0CA7,
	/* 14 */ 0x0F00,
	/* 15 */ 0x1088,
	/* 16 */ 0x1110,
	/* 17 */ 0x1088,
	/* 18 */ 0x0F00,
	/* 19 */ 0x0CA7,
	/* 1A */ 0x09C4,
	/* 1B */ 0x06AA,
	/* 1C */ 0x03AC,
	/* 1D */ 0x0114,
	/* 1E */ 0xFF17,
	/* 1F */ 0xFDCD,
	/* 20 */ 0xFD37,
	/* 21 */ 0xFD3D,
	/* 22 */ 0xFDB7,
	/* 23 */ 0xFE75,
	/* 24 */ 0xFF45,
	/* 25 */ 0x0000,
	/* 26 */ 0x008A,
	/* 27 */ 0x00D9,
	/* 28 */ 0x00EC,
	/* 29 */ 0x00D0,
	/* 2A */ 0x0097,
	/* 2B */ 0x0055,
	/* 2C */ 0x0018
};
#else
// Kaiser window FIR filter coefficeints.
// Roll off at 310 Hz, -60 dB @ 3100 Hz
FIR_COEFF Fir_Coeffs[N_FIR_TAPS] = {
/* 1   */ 0x0014,
/* 2   */ 0xFFFC,
/* 3   */ 0xFF00,
/* 4   */ 0xFD65,
/* 5   */ 0xFDDB,
/* 6   */ 0x042E,
/* 7   */ 0x10EB,
/* 8   */ 0x1E7A,
/* 9   */ 0x245F,
/* 10  */ 0x1E7A,
/* 11  */ 0x10EB,
/* 12  */ 0x042E,
/* 13  */ 0xFDDB,
/* 14  */ 0xFD65,
/* 15  */ 0xFF00,
/* 16  */ 0xFFFC,
/* 17  */ 0x0014
};
#endif

// FIR data for I and Q channels
FIR_DATA I_Channel_FIR;
FIR_DATA Q_Channel_FIR;

// Internals
int doFir(int sample, FIR_DATA *fir);

// initialize the FIR pointers
void FIRInit(void)
{
	I_Channel_FIR.chnum = I_CHANNEL;
	I_Channel_FIR.wrptr = 0;
	I_Channel_FIR.rdptr = 0;

	Q_Channel_FIR.chnum = Q_CHANNEL;
	Q_Channel_FIR.wrptr = 0;
	Q_Channel_FIR.rdptr = 0;
}

int RunFIR(int input_sample, int channel)
{
	switch (channel) {

	case I_CHANNEL:
		return(doFir(input_sample, &I_Channel_FIR));

	case Q_CHANNEL:
		return(doFir(input_sample, &Q_Channel_FIR));

	default:
		return 0;
	}
}

// run the FIR filter
int doFir(int sample, FIR_DATA *fir)
{
	fir->firdata[fir->wrptr] = sample;
	fir->rdptr = fir->wrptr;

	if (fir->wrptr == N_FIR_TAPS - 1)
		fir->wrptr = 0;
	else
		fir->wrptr = fir->wrptr + 1;

	int term1, term2;
	int macin, macout;
	long mac = 0l;

#if FIR_DEBUG
	int rd = fir->rdptr;
	if (fir->chnum == 0) fprintf(stderr, "[%02d:%02d->%04x] ", fir->wrptr, rd, sample&0xffff);
	for (int j = 0; j < N_FIR_TAPS; j++) {
		if (fir->chnum == 0) fprintf(stderr, "%04x ", fir->firdata[rd]&0xffff);
		if(rd == 0)
			rd = N_FIR_TAPS - 1;
		else
			rd = rd - 1;
	}
#endif

	for (int i = 0; i < N_FIR_TAPS; i++) {
		term1 = (int)fir->firdata[fir->rdptr];
		term2 = (int)Fir_Coeffs[i];
		macin = term1 * term2;
		mac += (long)macin;
		if (fir->rdptr == 0)
			fir->rdptr = N_FIR_TAPS - 1;
		else
			fir->rdptr = fir->rdptr - 1;
	}

	macout = (int)(mac >> MAC_SHIFT);

#if FIR_DEBUG
	if (fir->chnum == 0) fprintf(stderr, "= %08lx:%08x->%f\n", mac, macout, (double)macout/32767.0);
#endif

	return (macout);

}
