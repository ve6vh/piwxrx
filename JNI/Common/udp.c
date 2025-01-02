/*---------------------------------------------------------------------------
	Project:	      NOAA's ARC

	Module:		      Send UDP packet

	File Name:	      udp.c

	Author:		      Martin C. Alcock, VE6VH

	Revision:	      1.05

	Description:	  Manages the interface between the Java code and the RTL dongle. Processes
                  audio samples into two streams, one for UDP and the other for the FSK
                  demodulator.
                  
                  This program is free software: you can redistribute it and/or modify
                  it under the terms of the GNU General Public License as published by
                  the Free Software Foundation, either version 2 of the License, or
                  (at your option) any later version, provided this copyright notice
                  is included.
 
                  Copyright (c) 2018 Praebius Communications Inc.

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
#include <string.h>

#include "rtl.h"

//header feilds
#define	HDR1		0		// first header byte
#define	HDR2		1		// Second header
#define	SSEQ		2		// sequence ID
#define	TIMESTAMP	4		// timestamp
#define	SYNCSRC		8		// sync src ID

#define	MARK		0x80	// mark bit

short int sequence;         // current sequence from Java
unsigned int timestamp;     // time stamp
int codec;                  // codec of choice
int gain;

char *UDPbufferPtr  = NULL;

BOOL InitUDP(unsigned char *hdrPtr, int nhdrbytes, char *remoteip, int remotePort, char *myip, int myport, int codectype, int gainvalue)
{
	codec = codectype;
	gain = gainvalue;

	if (codec == CODEC_NONE) {
#ifdef WIN32
		_setmode(_fileno(stdout), _O_BINARY);
#endif
		return TRUE;
	}

    if ((UDPbufferPtr = (char *)malloc((size_t)(2 * RTP_HDRLEN + (MAXUDPLEN+SPARE)))) == NULL)
      return(FALSE);
  
	// process the header
	memcpy(UDPbufferPtr, hdrPtr, nhdrbytes);			// copy header into buffer
	UDPbufferPtr[HDR2] |= MARK;
    
	sequence = (((short int)UDPbufferPtr[SSEQ] & 0xff) << 8) 
				| ((short int)UDPbufferPtr[SSEQ + 1] & 0xff);
	timestamp = (((unsigned int)UDPbufferPtr[TIMESTAMP] & 0xff)<< 24)
				| ((unsigned int)UDPbufferPtr[TIMESTAMP + 1] & 0xff) << 16
				| ((unsigned int)UDPbufferPtr[TIMESTAMP + 2] & 0xff) << 8
				| ((unsigned int)UDPbufferPtr[TIMESTAMP + 3] & 0xff);

	// open the send socket: skip over leading "/" courtesy of Java
	remoteip++; myip++;
	DEBUGLEVEL(DEBUG_UDP)
		fprintf(stderr, "Remote IP: %s:%d: My IP %s:%d\n", remoteip, remotePort, myip, myport);

	if (!OpenSocket((char *)remoteip, remotePort, (char *)myip, myport)) {
		DEBUGLEVEL(DEBUG_UDP)
			fprintf(stderr, "Open Socket Failed\n");
		return(FALSE);
	}

    return TRUE;
  
}

/*---------------------------------------------------------------------------

	FUNCTION:	SendUDPPacket

	INPUTS:		buffer pointers

	OUTPUTS:	FALSE

	DESCRIPTION:	free the buffers, if allocated

---------------------------------------------------------------------------*/
BOOL SendUDPPacket(RTL_SAMPLE *PipeBuffer, int samplesread)
{
    int decimlength = PCMEncode(PipeBuffer, samplesread, &UDPbufferPtr[RTP_HDRLEN], codec, gain);

	// if we are writing to stdout, just return here...
	if (codec == CODEC_NONE)
		return TRUE;

	Send(UDPbufferPtr, decimlength +RTP_HDRLEN);

	DEBUGLEVEL(DEBUG_UDP)
		fprintf(stderr, "s: %d..", decimlength + RTP_HDRLEN);

	timestamp += decimlength;
	sequence += 1;
	UDPbufferPtr[HDR2] &= ~MARK;
	UDPbufferPtr[SSEQ] = (sequence >> 8) & 0xff;
	UDPbufferPtr[SSEQ + 1] = sequence & 0xff;
	UDPbufferPtr[TIMESTAMP] = (timestamp >> 24) & 0xff;
	UDPbufferPtr[TIMESTAMP + 1] = (timestamp >> 16) & 0xff;
	UDPbufferPtr[TIMESTAMP + 2] = (timestamp >> 8) & 0xff;
	UDPbufferPtr[TIMESTAMP + 3] = timestamp & 0xff;

	DEBUGLEVEL(DEBUG_UDP)
		fprintf(stderr, "%d bytes sent: ts %d: seq %d\n", decimlength, timestamp, sequence);

	return TRUE;
}

/*---------------------------------------------------------------------------

	FUNCTION:	CloseUDP

	INPUTS:		none

	OUTPUTS:	none

	DESCRIPTION:	Close socket and deallocate memory

---------------------------------------------------------------------------*/
void CloseUDP(void)
{
	if (codec != CODEC_NONE) {
		if (UDPbufferPtr != NULL)
			free(UDPbufferPtr);
		CloseSocket();
	}
}
