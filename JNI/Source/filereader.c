/*---------------------------------------------------------------------------

	Project:	      PiWxRx Weather receiver

	Module:		      File and USB reader

	File Name:		  filereader.c

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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "rtl.h"

// operating Modes
#define	NO_MODE				-1			// no mode specified
#define	FILE_MODE			0			// reading a file
#define	USB_MODE_BY_ID		1			// reading a USB device
#define USB_MODE_BY_CARD	2			// USB by card numner

char *filename;						// filename

char *modes[] = {
		"file",
		"usb by ID",
		"usb by Card"
};

unsigned char *readBuf;
RTL_SAMPLE *writeBuf;

USB_AUDIO_DEV usbdev;

int main(int argc, char *argv[])
{
	int bytesRead;
	int shift1 = 8, shift2 = 0;
	int debug = 0;
	FILE *infile = stdin;
	int gain = 0;
	BOOL running = TRUE;
	int mode = NO_MODE;
	int rdBufSize;
	int wrBufSize;

	for (int i = 0; i < argc; i++) {

		if (argv[i][0] == '-')
			switch (argv[i][1]) {

			case 'f':
				filename = argv[++i];
				rdBufSize = FILE_READ_SIZE*sizeof(RTL_SAMPLE);
				wrBufSize = FILE_READ_SIZE;
				mode = FILE_MODE;
				break;

			case 'u':
				rdBufSize = USB_READ_SIZE*sizeof(RTL_SAMPLE);
				wrBufSize = USB_READ_SIZE;

				switch(argv[i][2])	{

				case 'i':
					mode = USB_MODE_BY_ID;
					sscanf(argv[++i], "%x", (unsigned int *) &usbdev.idVendor);
					sscanf(argv[++i], "%x", (unsigned int *) &usbdev.idProduct);
					break;

				case 'c':
					mode = USB_MODE_BY_CARD;
					sscanf(argv[++i], "%x", (unsigned int *) &usbdev.cardNum);
					break;

				default:
					fprintf(stderr, "Usage: -ui <vendor> <product> | -uc <card>\n");
					exit(100);
				}
				break;

			case 'g':
				sscanf(argv[++i], "%d", &gain);
				break;

			case 'd':
				debug++;
				break;

			case 'l':
				shift1 = 0; shift2 = 8;
				break;

			case 'b':
				shift1 = 8; shift2 = 0;
				break;

			default:
				fprintf(stderr, "Usage: filereader [ -f <file> -[b|l] | [ -ui <vendor> <product> | -uc <card> ]]-g <gain> -d\n");
				exit(100);
			}

	}

	// allocate the buffers
	if((readBuf=malloc(rdBufSize)) == NULL)	{
		fprintf(stderr, "Cannot alloc memory for read buffer\n");
		exit(200);
	}
	if((writeBuf=malloc(wrBufSize)) == NULL)	{
		fprintf(stderr, "Cannot alloc memory for read buffer\n");
		exit(200);
	}

	// check the mode and open the device
	switch(mode)		{

	case NO_MODE:
		fprintf(stderr, "No input mode specified - aborting\n");
		exit(100);

	case FILE_MODE:
		if((infile=fopen(filename, "rb")) == NULL)	{
			fprintf(stderr, "Cannot open input file %s\n", filename);
			exit(100);
		}
		break;

	case USB_MODE_BY_ID:
	case USB_MODE_BY_CARD:
		InitUSB(debug ? DEBUG_USB : 0);
		if(mode == USB_MODE_BY_ID)	{
			if(!FindUSBDevice(&usbdev))	{
				fprintf(stderr, "Cannot find card number for USB device %04x:%04x\n", usbdev.idVendor, usbdev.idProduct);
				exit(100);
			}
		}
		usbdev.recordSize = USB_READ_SIZE;
		if(!OpenUSBDevice(&usbdev)) {
			fprintf(stderr, "Cannot open USB device\n");
			exit(100);
		}
		if(debug)
			fprintf(stderr, "USB device successfully opened\n");
		shift1 = 0; shift2 = 8;				// set LE by default
		break;
	}

	if (debug)
		fprintf(stderr, "Filereader started in %s mode\n", modes[mode]);

#ifdef _WIN32
		_setmode(_fileno(stdin), _O_BINARY);
		_setmode(_fileno(stdout), _O_BINARY);
#endif


	while (running) {
		if(mode == FILE_MODE)
			bytesRead = (int)fread(readBuf, sizeof(char), FILE_READ_SIZE*sizeof(RTL_SAMPLE), infile);
		else
			bytesRead = readUSB(readBuf, usbdev.recordSize);
		
		unsigned char *inbuf = readBuf;
		RTL_SAMPLE *outbuf = writeBuf;

		if (debug)
			fprintf(stderr, "Filereader: Read %d bytes\n", bytesRead);

		if (bytesRead == 0) {
			fprintf(stderr, "End of file reached\n");
			running = FALSE;
			break;
		}

		// make sure we have an even number of samples...
		if (bytesRead & 1)
			bytesRead--;

		RTL_SAMPLE insample;
		int decimsample, samplez1 = 0;
		for (int i = 0; i < bytesRead; i += 2) {
			insample = (((RTL_SAMPLE)*inbuf++) << shift1);
			insample |= (((RTL_SAMPLE)*inbuf++) << shift2);
			decimsample = (int)insample + (int)samplez1;
		
			if((i>>1) & 1)
				*outbuf++ = (RTL_SAMPLE) ((decimsample>>1) & 0xffff) << gain;
			samplez1 = insample;
		}
		int samplestowrite = bytesRead / 4;

		fwrite((void *)writeBuf, sizeof(RTL_SAMPLE), samplestowrite, stdout);
		if (debug)
			fprintf(stderr, "Filereader: Wrote %d 16-bit samples\n", samplestowrite);

	}
	fprintf(stderr, "Filereader exiting\n");
	return 0;
}
