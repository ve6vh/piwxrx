/*---------------------------------------------------------------------------
	Project:	      PiWxRx Weather receiver

	Module:		      Quadrature oscillator

	File Name:		  osc.c

	Author:		      Martin C. Alcock, VE6VH

	Revision:	      1.05

	Description:	  This module implements an complex oscillator for the fsk mixer injection

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
#include <math.h>

#include "rtl.h"

#define	PI	3.141592653562795
#define	FIR_DEBUG	0

#define LUT_SIZE		2048			// sizeof(lookup table)
#if FIR_DEBUG
#define	INJ_FREQ		44				// phase advance for 44=260Hz;563=3298Hz
#else
#define	INJ_FREQ		311				// phase advance for 1822 Hz
#endif
OSC_VALUE oscLut[LUT_SIZE];

int		i_Phase = 0;
int		q_Phase = (3* (LUT_SIZE)) / 4;

void InitOsc(void)
{
	for (int i = 0; i < LUT_SIZE; i++)
		oscLut[i] = (OSC_VALUE)((cos(2.0 * PI*(double)i / (double)LUT_SIZE))*32767.0);
}

RTL_SAMPLE RunOsc(int channel)
{
	OSC_VALUE oscout;

	switch (channel) {

	case I_CHANNEL:
		oscout = oscLut[i_Phase];
		i_Phase = (i_Phase + INJ_FREQ) & (LUT_SIZE -1);
		break;

	case Q_CHANNEL:
		oscout = oscLut[q_Phase];
		q_Phase = (q_Phase + INJ_FREQ) & (LUT_SIZE - 1);;
		break;

	default:
		return 0;

	}
	return((RTL_SAMPLE)oscout);
}
