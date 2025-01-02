/*---------------------------------------------------------------------------
	Project:	      PiWxRx Weather receiver

	Module:		      Signal processing module for ADSP demod

	File Name:		 demod.c

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
#endif

#include <stdio.h>
#include <stdlib.h>

#include "rtl.h"

// local defines

#define		N_CORR_BITS		BITSPERBYTE*2
#define		CORR_LENGTH		BIT_DIVISOR*N_CORR_BITS
#define		DEMOD_DLY_LEN	64

// correlator stuff
DATA_BIT correlator[CORR_LENGTH];
DATA_BIT sync_bits[N_CORR_BITS] = {
	1, 0, 1, 0, 1, 0, 1, 1,					// Hex AB
	1, 0, 1, 0, 1, 0, 1, 1					// Hex AB
};

RTL_SAMPLE I_demod_dly[DEMOD_DLY_LEN];
RTL_SAMPLE Q_demod_dly[DEMOD_DLY_LEN];

int curr_index = 0;

DATA_BIT last_demod_bit = 0;

// look for the sync code in the correlator: AB (16)
BOOL SyncCorrelator(DATA_BIT databit)
{
	// bits are LSB first: so shift left to right
	for (int i = CORR_LENGTH - 1; i > 0; i--)
		correlator[i] = correlator[i - 1];
	correlator[0] = databit;

	for(int i=0;i < N_CORR_BITS; i++)
		if (correlator[i*BIT_DIVISOR] != sync_bits[i]) {
			return FALSE;
		}
	return TRUE;
}

int PhaseDiscrim(int Iout, int Qout)
{
	int phase;
	long lphase;
	int previous_bit_index = (curr_index - 12) & (DEMOD_DLY_LEN - 1);

	I_demod_dly[curr_index] = Iout;
	Q_demod_dly[curr_index] = Qout;

	int Iprev = I_demod_dly[previous_bit_index];
	int Qprev = Q_demod_dly[previous_bit_index];

	lphase = (long)Iprev * (long)Qout;
	lphase -= (long)Iout * (long)Qprev;

	curr_index = (curr_index + 1) & (DEMOD_DLY_LEN - 1);

	phase = (int)(lphase)>> 11;
	return phase;

}
