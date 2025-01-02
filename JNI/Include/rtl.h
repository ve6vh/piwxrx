/*---------------------------------------------------------------------------
	Project:	      NOAA's ARC

	Module:		      System wide definitions

	File Name:		  rtl.h

	Author:		      Martin C. Alcock, VE6VH

	Revision:	      1.05

	Description:	  Contains definitions used by the various modules

					  This program is free software: you can redistribute it and/or modify
					  it under the terms of the GNU General Public License as published by
					  the Free Software Foundation, either version 2 of the License, or
					  (at your option) any later version, provided this copyright notice
					  is included.

					  Copyright (c) 2018 Praebius Communications Inc.

	Revision History:
	 
---------------------------------------------------------------------------*/
#define _CRT_SECURE_NO_WARNINGS	

#pragma once

// define windows types and constants
#ifdef _WIN32
#include <windows.h>
#define	sprintf	sprintf_s
// linux constants
#else
typedef unsigned short BOOL;
#define FALSE     0
#define TRUE      1

char *geterrno(int errnum);
#endif

#include <stdint.h>

// debug modes
#define	DEBUG_NONE		0x0000		// placeholder for no debug
#define	DEBUG_MSGS		0x0001		// messages only
#define	DEBUG_OSC		0x0002		// oscillator output
#define	DEBUG_LPF		0x0004		// low pass filter output
#define	DEBUG_DEMOD		0x0008		// demod output
#define	DEBUG_UDP		0x0010		// udp debug
#define DEBUG_WRITE		0x0020		// write out samples only
#define DEBUG_BITSHIFT		0x0040		// bit shifter
#define	DEBUG_BYTEOUT		0x0080		// byte decoder
#define	DEBUG_SYNC		0x0100		// sync debug
#define	DEBUG_JNI		0x0200		// debug JNI
#define	DEBUG_USB		0x0400		// debug USB
#define	DEBUG_DEEMPHASIS	0x0800		// debug with deemphasis

#define	DEBUGPRINTF(c)	if(debuglevel&DEBUG_MSGS) fprintf(stderr, "%s", c);
#define	DEBUGLEVEL(x)	if(debuglevel&x)
#define	BACKGDEBUG(x)	if(s->debuglevel&x) 

extern int debuglevel;

// local data types
typedef unsigned char	CODEC_BYTE;         // codec data byte
typedef int16_t			RTL_SAMPLE;         // sample from RTL dongle
typedef int16_t			FIR_COEFF;          // filter coefficient
typedef int16_t			OSC_VALUE;			// oscillator values
typedef uint8_t			DEMOD_BYTE;			// last demodulated byte
typedef uint8_t			DATA_BIT;			// data bit type

#define	MAX_FIR_TAPS		17

// NOAA protocol equates
#define		NO_BYTE		0xED
#define		SYNC_BYTE	0xAB
#define		EOM_BYTE	0x00

#ifdef __cplusplus
extern "C" {
#endif

// decimation rates
#define		RAW_SAMPLE_RATE		48000	  // sample rate of USB device
#define		SAMPLE_RATE			24000	  // audio sample rate
#define		RAW_DECIM_RATE		(RAW_SAMPLE_RATE/SAMPLE_RATE)
#define		AUDIO_DECIM			3         // audio decimate by 3
#define		FSK_DECIM			2         // FSK decimate by 2
#define		PIPE_DECIM_RATE		1

// stdio pipe defines
#define		PIPE_READ_LEN		720		  // 30 ms @ 24KHz sample rate
#define		PIPE_READ_SIZE		PIPE_READ_LEN*PIPE_DECIM_RATE

#define		USB_FRAME_SIZE		2		  // bytes in a frame
#define		USB_READ_SIZE		1024	  // sizeof frame to read from USB
#define		FILE_READ_SIZE		(PIPE_READ_LEN*RAW_DECIM_RATE)

#define		MAXFSKLEN			(PIPE_READ_LEN/AUDIO_DECIM)
#define		MAXUDPLEN 			(PIPE_READ_LEN/FSK_DECIM)
#define		SPARE				100
#define		RTP_HDRLEN			12
#define		TIMER_VALUE			30

#define		MSTONS(x)		((long long)x*(long long)1000000)
// codec types in SDP
#define		CODEC_NONE			-1		// no codec
#define		CODEC_PCMU	        0	    // G711 u-law 
#define		CODEC_GSM	        3	    // GSM 
#define		CODEC_G723	        4	    // G723 
#define		CODEC_DVI4	        5	    // DBI4 
#define		CODEC_LPC	        7	    // LPC 
#define		CODEC_PCMA	        8	    // G711 a-law 
#define		CODEC_G722	        9	    // G722
#define		CODEC_QCELP	        12		// QCELP
#define		CODEC_CN	        13		// CN
#define		CODEC_G728	        15		// G728
#define		CODEC_G729	        18		// G729

#define		BITSPERBYTE			8		// demod bits/byte
#define		BIT_DIVISOR			23		// bit time divisor
#define		SWALLOW_CTR			23		// swallow ctr init
#define		SYNC_BYTE			0xAB	// sync byte

// sample rates
#define CODEC_SAMPLE_RATE		8000		// all codec sample rates

// signal processing constants
#define I_CHANNEL				0             // I demod channel
#define Q_CHANNEL				1             // Q demod channel
#define	SAMPLE_BFRSIZ			4096		  // sizeof (sample buffer)
#define	BIT_TIME				23			  // nominal bit time
#define DUAL_MODULUS			24			  // interval for dual modulus prescaler	

#define BUFFER_EMPTY(x)		((x.rdptr) == (x.wrptr))

typedef uint16_t	USB_DEV_ID;			// device ID

// USB device ID's
typedef struct usb_dev_t	{
	// user specified
	USB_DEV_ID	idVendor;				// verdor ID
	USB_DEV_ID	idProduct;				// product ID
	uint32_t	partID;					// ID of the part found
	uint8_t		cardNum;				// card number
	int			recordSize;				// record size
} USB_AUDIO_DEV;

// from RTL.c: these are links in from the JNI
BOOL InitRTL(char *cmdline, void (*rx_func)(DEMOD_BYTE x), int debuglevel);
BOOL RunRTL(void);
void StopRTL(void);
void ClrFSKSync(void);
BOOL StartUDP(unsigned char *hdrPtr, int nhdrbytes, char *remoteip, int remotePort, char *myip, int myport, int codec, int gain);
void StopUDP(void);

// from databuffer.c
void databuffer_init(void);
void databuffer_put(DEMOD_BYTE byterx);
DEMOD_BYTE databuffer_get(void);

// from UDP.c
BOOL InitUDP(unsigned char *hdrPtr, int nhdrbytes, char *remoteip, int remotePort, char *myip, int myport, int codec, int gain);
BOOL SendUDPPacket(RTL_SAMPLE *PipeBuffer, int bytesread);
void CloseUDP(void);

// from child.c
BOOL InitPipes(void);
BOOL initChildProcess(char *cmdline);
BOOL CreateChildProcess(char *szCmdline);
void CloseChildProcess();
int ReadFromPipe(RTL_SAMPLE *buffer, int bfrsiz);

// from UDPSocket.cpp
BOOL OpenSocket(char *remoteip, int remoteport, char *myip, int myport);
BOOL Send(const char *buffer, int nbytes);
void CloseSocket(void);

// from codec.c
int PCMEncode(RTL_SAMPLE *buffer, int len, char *encoded_buf, int codec, int gain);
int PCMDecode(CODEC_BYTE *inbuf, RTL_SAMPLE *buffer, int len, int codec);
int PipeDecimate(RTL_SAMPLE *Buffer, int readlen, int decimlen);

// from g711.c
CODEC_BYTE linear2alaw(int pcm_val);
int alaw2linear(CODEC_BYTE	a_val);
CODEC_BYTE linear2ulaw(int pcm_val);
int ulaw2linear(CODEC_BYTE	u_val);
CODEC_BYTE alaw2ulaw(CODEC_BYTE aval);
CODEC_BYTE ulaw2alaw(CODEC_BYTE uval);

// from FSKdsp.c
void DSPInit(void (*rx_func)(DEMOD_BYTE x), int debuglevel);
void SetDebugLevel(int level);
void DSPDemod(RTL_SAMPLE *PipeBufferPtr, int bytesread);
void DSPStop(void);
void DSPClearSync(void);

// From Demod.c
BOOL SyncCorrelator(DATA_BIT databit);
int PhaseDiscrim(int Iout, int Qout);

// from fir.c
void FIRInit(void);
int RunFIR(int input_sample, int channel);
int RunDeemph(int input_sample);

// from osc.c
void InitOsc(void);
RTL_SAMPLE RunOsc(int channel);

// from usb.c
BOOL InitUSB(int debug);
BOOL FindUSBDevice(USB_AUDIO_DEV *usrdev);
BOOL OpenUSBDevice(USB_AUDIO_DEV *usrdev);
int readUSB(void *buffer, int len);

//from errno.c
char *geterrno(int errnum);

#ifdef __cplusplus
}
#endif
