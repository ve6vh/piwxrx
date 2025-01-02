/*---------------------------------------------------------------------------
	Project:	      PiWxRx  - a Pi Weather receiver

	Module:		      USB routines

	File Name:		  usb.c

	Author:		      Martin C. Alcock

	Revision:	      3.01

	Description:


                  Copyright (c) 2018-2022 Praebius Communications Inc.

	Revision History:

---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>
#include <alsa/asoundlib.h>

#include "rtl.h"

struct libusb_context *context=NULL;			// USB Context
int debuglevel;

// ALSA structures
snd_pcm_t *pcm_handle;
snd_pcm_stream_t capture= SND_PCM_STREAM_CAPTURE;
snd_pcm_hw_params_t *hwparams;
char pcm_name[100];

// internals
BOOL getUSBCardNum(int ndev, char *usbId, USB_AUDIO_DEV *usrdev);

/*---------------------------------------------------------------------------

	FUNCTION:		InitHAL

	INPUTS:			HAL_INFO *

	OUTPUTS:		error code or 0

	DESCRIPTION:	find the USB device by ID, and open the files

---------------------------------------------------------------------------*/
BOOL InitUSB(int debug)
{
	debuglevel = debug;
	libusb_init(&context);					// initalize the USB library

	return TRUE;
}

/****************************************************************************
 * 			Local Methods
 ***************************************************************************/
BOOL FindUSBDevice(USB_AUDIO_DEV *usrdev)
{
	char usbId[100];

	// first get a list of devices
	libusb_device **usb_dev = NULL;

	int ndev = libusb_get_device_list(context, &usb_dev);
	if(debuglevel&DEBUG_USB)
		fprintf(stderr, "get_device returned %d\n", ndev);

	if(ndev == 0)
		return FALSE;

	for(int i=0;i<ndev; i++)	{
		libusb_device *dev = usb_dev[i];
		struct libusb_device_descriptor desc = {0};

		int ndesc = libusb_get_device_descriptor(dev, &desc);
		if(ndesc != 0)	{
			if(debuglevel&DEBUG_USB)
				fprintf(stderr, "get_device_descriptor returned %d\n", ndesc);
			return FALSE;
		}

		if(debuglevel&DEBUG_USB)
			fprintf(stderr, "Device %04x:%04x\n", desc.idVendor, desc.idProduct);

		// see if this is the device that we want...
		if((usrdev->idVendor == desc.idVendor) && (usrdev->idProduct == desc.idProduct))	{
			sprintf(usbId, "%04x:%04x", usrdev->idVendor, usrdev->idProduct);
			if(debuglevel&DEBUG_USB)
				fprintf(stderr, "Selected Device %s\n", usbId);
			usrdev->partID = ((usrdev->idVendor) << 16) | usrdev->idProduct;
			return(getUSBCardNum(ndev, usbId, usrdev));
		}
	}
	return(FALSE);
}

/*
 * return the card number for a given ID
 */
BOOL getUSBCardNum(int ndev, char *usbId, USB_AUDIO_DEV *usrdev)
{
	// find the card number where this device is:
	for(int i=0;i<ndev;i++)	{
		char fname[100], devID[50];
		FILE *fp;
		sprintf(fname, "/proc/asound/card%d/usbid", i);
		if((fp=fopen(fname, "r")) != NULL)	{
			fgets(devID, sizeof(devID)-1, fp);
			if(devID[strlen(devID)-1] == '\n')
				devID[strlen(devID)-1] = '\0';
			if(!strcmp(devID, usbId))	{
				if(debuglevel&DEBUG_USB)
					fprintf(stderr, "Found %s on card %d\n", devID, i);
				usrdev->cardNum = i;
				return TRUE;
			}
		}
	}
	if(debuglevel&DEBUG_USB)
		fprintf(stderr, "Could not find USB card number %s\n", usbId);

	return FALSE;
}

/*
 * Open a USB device
 */
BOOL OpenUSBDevice(USB_AUDIO_DEV *usrdev)
{
	int errno;

	// Step 1: allocate memory for a hwparams struct
	snd_pcm_hw_params_alloca(&hwparams);


	// Step 2: open the device
	sprintf(pcm_name, "plughw:%d,0", usrdev->cardNum);
	if((errno=snd_pcm_open(&pcm_handle, pcm_name, capture, 0)) < 0)	{
		fprintf(stderr, "Error %s opening device %s\n", snd_strerror(errno), pcm_name);
		return FALSE;
	}

	// Step 3: get the current configuration
	if((errno=snd_pcm_hw_params_any(pcm_handle, hwparams)) < 0)	{
		fprintf(stderr, "Error %s getting parameters for %s\n", snd_strerror(errno), pcm_name);
		return FALSE;
	}

	// set non-interleaved access
	if((errno=snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_NONINTERLEAVED)) < 0)	{
		fprintf(stderr, "Error %s setting access mode on %s\n", snd_strerror(errno), pcm_name);
		return FALSE;
	}

	// set blocking mode
	if((errno=snd_pcm_nonblock(pcm_handle, 0)) < 0)	{
		fprintf(stderr, "Error %s setting blocking mode on %s\n", snd_strerror(errno), pcm_name);
		return FALSE;
	}

	//  Step 3d: set resampling
	if(snd_pcm_hw_params_set_rate_resample(pcm_handle, hwparams, 1) < 0)	{
			fprintf(stderr, "Error setting resampling\n");
			return FALSE;
	}

	// now setup the HW params struct
	// Step 3a: set the endian format
	if((errno=snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE)) < 0)	{
		fprintf(stderr, "Error %s setting format on %s\n", snd_strerror(errno), pcm_name);
		return FALSE;
	}

	//  Step 3b: set the sample rate
	unsigned int sample_rate = RAW_SAMPLE_RATE;
	if(snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &sample_rate, 0) < 0)	{
		fprintf(stderr, "Error setting sample rate\n");
		return FALSE;
	}
	if(sample_rate != (unsigned int)RAW_SAMPLE_RATE)
		fprintf(stderr, "Sample rate set to %d, some functions may not work\n", sample_rate);

	//  Step 3c: set the number of channels
	if(snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 1) < 0)	{
			fprintf(stderr, "Error setting number of channels\n");
			return FALSE;
	}

	//  Step 3e: set the period and buffer size
	unsigned int per_size = usrdev->recordSize;
	if((errno=snd_pcm_hw_params_set_periods(pcm_handle, hwparams, per_size, 0)) < 0)	{
		fprintf(stderr, "Error %s setting period size on %s\n", snd_strerror(errno), pcm_name);
		return FALSE;
	}

	// set buffer to 4 periods
	snd_pcm_uframes_t buf_size = 4;
	if(snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams, &buf_size) < 0)	{
		fprintf(stderr, "Error setting buffer size\n");
		return FALSE;
	}

	//  Step 4: now set the params
	if(snd_pcm_hw_params(pcm_handle, hwparams) < 0)	{
		fprintf(stderr, "Error setting hardware parameters\n");
		return FALSE;
	}

	// step 4: start the device
	if((errno=snd_pcm_prepare(pcm_handle)) < 0)	{
		fprintf(stderr, "Error starting device: %s\n", snd_strerror(errno));
		return FALSE;
	}

	return TRUE;
}

int nframesRead=0;

/*
 * Read a USB device. The length specification is in frames, but it returns
 * the record length in bytes to be compatible with the file mode
 */
int readUSB(void *buffer, int len)
{
	void *frame = buffer;
	snd_pcm_uframes_t nframes = len;
	snd_pcm_sframes_t nread = 0;

	// continue while in error or no data state
	while(nread <= 0)	{

		nread = snd_pcm_readn(pcm_handle, &frame, nframes);

		// broken pipe error
		if(nread == -EPIPE)	{
			fprintf(stderr, "Buffer overrun..attempting to recover\n");
			sleep(1);
			snd_pcm_prepare(pcm_handle);
		} else {
			if(nread < 0)	{
				fprintf(stderr, "USB Read error %ld: %s\n",  nread, snd_strerror(errno));
				fprintf(stderr, "%d frames were read\n", nframesRead);
				return 0;
			}
		}
	}
	nframes++;
	return nread*USB_FRAME_SIZE;
}
