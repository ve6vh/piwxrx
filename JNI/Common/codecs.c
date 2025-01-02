/*---------------------------------------------------------------------------
	Project:	      PiWxRx Weather receiver

	Module:		      G711 codecs

	File Name:	      codecs.c

	Author:		      Martin C. Alcock, VE6VH

	Revision:	      1.05

	Description:		Implements a G711 u or a law codec

					This program is free software: you can redistribute it and/or modify
					it under the terms of the GNU General Public License as published by
					the Free Software Foundation, either version 2 of the License, or
					(at your option) any later version, provided this copyright notice
					is included.

				  Copyright (c) 2018-2022 Praebius Communications Inc.

	Revision History:

---------------------------------------------------------------------------*/

#include <stdint.h>

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "rtl.h"

#define	RAW_MODE		0			// write out raw samples

// internals
void stdioOutEncode(RTL_SAMPLE *buffer, int len, int gain);

int G711uLawEncode(RTL_SAMPLE *buffer, CODEC_BYTE *outbuf, int len, int gain);
void G711uLawDecode(CODEC_BYTE *inbuf, int16_t *outbuf, int len);

int G711aLawEncode(RTL_SAMPLE *buffer, CODEC_BYTE *outbuf, int len, int gain);
void G711aLawDecode(CODEC_BYTE *inbuf, int16_t *buffer, int len);

RTL_SAMPLE deemph(RTL_SAMPLE input);

CODEC_BYTE *encoded_buf;

int32_t state;

int PCMEncode(RTL_SAMPLE *buffer, int len, char *encoded_buf, int codec, int gain)
{

	DEBUGPRINTF("Entered Codec output\n");

	switch (codec) {

	case CODEC_NONE:
		stdioOutEncode(buffer, len, gain);
		break;

	case CODEC_PCMU:
		return(G711uLawEncode(buffer, (CODEC_BYTE *)encoded_buf, len, gain));
		break;

	case CODEC_PCMA:
		return(G711aLawEncode(buffer, (CODEC_BYTE *)encoded_buf, len, gain));
		break;

	}
	return 0;
}

int PCMDecode(CODEC_BYTE *inbuf, RTL_SAMPLE *buffer, int len, int codec)
{

	switch (codec) {

	case CODEC_PCMU:
		G711uLawDecode(inbuf, buffer, len);
		break;

	case CODEC_PCMA:
		G711aLawDecode(inbuf, buffer, len);
		break;

	}
	return(len);
}

// do the decimation but just write to stdout
void stdioOutEncode(RTL_SAMPLE *buffer, int len, int gain)
{
#if RAW_MODE
	fwrite(buffer, len, sizeof(RTL_SAMPLE), stdout);
#else
	RTL_SAMPLE *outbuf = (RTL_SAMPLE *)malloc(len);
	RTL_SAMPLE *encoded_buf = outbuf;
	int encoded_len = 0;
	for (int i = 0; i < len/3; i++) {
		int decim_sample;
		decim_sample = (int)*buffer++;
		decim_sample += (int)*buffer++;
		decim_sample += (int)*buffer++;
		*encoded_buf++ = (RTL_SAMPLE)((decim_sample/3) & 0xffff);
		encoded_len++;
	}
	fwrite(outbuf, encoded_len, sizeof(RTL_SAMPLE), stdout);
	free(outbuf);
#endif
}

// Encode output to G711 u Law: decimate input by three to make 8KHz sample rate
int G711uLawEncode(RTL_SAMPLE *buffer, CODEC_BYTE *outbuf, int len, int gain)
{
	int newlen = len / 3;
	for (int i = 0; i < newlen; i++) {
        int decim_sample;
		decim_sample = (int)*buffer++;
		decim_sample += (int)*buffer++;
		decim_sample += (int)*buffer++;
		if(debuglevel & DEBUG_DEEMPHASIS)
			*outbuf++ = linear2ulaw(deemph((RTL_SAMPLE)((decim_sample / 3) & 0xffff) << gain));
		else
			*outbuf++ = linear2ulaw((RTL_SAMPLE)((decim_sample / 3) & 0xffff) << gain);
	}
	return(newlen);
}

// decode a buffer of uLAW
void G711uLawDecode(CODEC_BYTE *inbuf, int16_t *buffer, int len)
{
	for (int i = 0; i < len; i++) {
		*buffer++ = ulaw2linear(*inbuf++);
	}
}

// Encode output to G711 a Law
int G711aLawEncode(RTL_SAMPLE *buffer, CODEC_BYTE *outbuf, int len, int gain)
{
	int newlen = len / 3;
	for (int i = 0; i < newlen; i++) {
		int decim_sample;
		decim_sample = (int)*buffer++;
		if(debuglevel & DEBUG_DEEMPHASIS)
			*outbuf++ = linear2alaw(deemph((RTL_SAMPLE)((decim_sample / 3) & 0xffff) << gain));
		else
			*outbuf++ = linear2alaw((RTL_SAMPLE)((decim_sample / 3) & 0xffff) << gain);
	}
	return(newlen);
}

// decode a buffer of uLAW
void G711aLawDecode(CODEC_BYTE *inbuf, int16_t *buffer, int len)
{
	for (int i = 0; i < len; i++) {
		*buffer++ = alaw2linear(*inbuf++);
	}
}

// decimate the input samples
int PipeDecimate(RTL_SAMPLE *Buffer, int readlen, int decimlen)
{
	RTL_SAMPLE *newSample = Buffer;

	if(readlen <= decimlen)
		return readlen;

	for(int i=0;i<readlen;i++)	{
		RTL_SAMPLE dsamp = *Buffer++;
		dsamp += *Buffer++;
		*newSample++ = (dsamp/2);
	}
	return decimlen;
}

// deemphasis filter stolen from USBRADIO.c

/* Perform standard 6db/octave de-emphasis */
RTL_SAMPLE deemph(RTL_SAMPLE input)
{
int16_t coeff00 = 6878;
int16_t coeff01 = 25889;
int32_t accum; /* 32 bit accumulator */

        accum = input;
        /* YES! The parenthesis REALLY do help on this one! */

        state = accum + ((state * coeff01) >> 15);
        accum = (state * coeff00);

        /* adjust gain so that we have unity @ 1KHz */
        return((accum >> 14) + (accum >> 15));
}
