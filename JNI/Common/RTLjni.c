/*---------------------------------------------------------------------------
	Project:	      PiWxRx Weather receiver

	Module:		      Main RTL JNI layer code

	File Name:		  RTLjni.c

	Author:		      Martin C. Alcock

	Revision:	      1.05

	Description:	  Contains the Java interface to the native routines. All are repeated
					  in rtl.c to keep the differences to a minimum.
						

					  This program is free software: you can redistribute it and/or modify
					  it under the terms of the GNU General Public License as published by
					  the Free Software Foundation, either version 2 of the License, or
					  (at your option) any later version, provided this copyright notice
					  is included.

					  Copyright (c) 2018-2022 Praebius Communications Inc.

	Revision History:

---------------------------------------------------------------------------*/

#include "jni_md.h"
#include "jni.h"
#include "PiJNI_RTLsdrJNI.h"
#include "rtl.h"

// callback for data rx puts data in the buffer 
static void byteRx(DEMOD_BYTE data)
{
	databuffer_put(data);
}

/*------------------------------------------------------------------------------------------*/
/*							Methods for RTL-SDR												*/
/*------------------------------------------------------------------------------------------*/
JNIEXPORT jboolean JNICALL Java_PiJNI_RTLsdrJNI_init
(JNIEnv *env, jobject o, jstring cmd, jint debuglevel)
{
	const char *cmdline = (*env)->GetStringUTFChars(env, cmd, NULL);
	DEBUGLEVEL(DEBUG_JNI)
		fprintf(stderr, "entered init\n");

	fprintf(stderr, "Starting: %s at debug level %x\n", cmdline, debuglevel);
	if (!InitRTL((char *)cmdline, &byteRx, debuglevel))	{
		fprintf(stderr, "Exec failed\n");
		return JNI_FALSE;
	}

	DEBUGLEVEL(DEBUG_JNI)
		fprintf(stderr, "Exec successful\n");

	databuffer_init();

	return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_PiJNI_RTLsdrJNI_runRTL
(JNIEnv *env, jobject o)
{
	RunRTL();
}

JNIEXPORT void JNICALL Java_PiJNI_RTLsdrJNI_stopRTL
(JNIEnv *env, jobject o)
{
	StopRTL();
}

/*------------------------------------------------------------------------------------------*/
/*							Methods for FSK Data receiver 									*/
/*------------------------------------------------------------------------------------------*/
// Clear FSK sync
JNIEXPORT void JNICALL Java_PiJNI_RTLsdrJNI_clrFSKSync
(JNIEnv *env, jobject o)
{
	ClrFSKSync();
}

// get a byte
JNIEXPORT jbyte JNICALL Java_PiJNI_RTLsdrJNI_getRxByte
(JNIEnv *env, jobject o)
{
	jbyte retval;

	retval = (jbyte)databuffer_get();

	return retval;

}


/*------------------------------------------------------------------------------------------*/
/*							Methods for UDP 												*/
/*------------------------------------------------------------------------------------------*/
JNIEXPORT jboolean JNICALL Java_PiJNI_RTLsdrJNI_startUDP
(JNIEnv *env, jobject o, jbyteArray hdr, jint jhdrlen, jstring remoteIP, jint remotePort, 
	jstring myIP, jint myport, jint codec, jint gain)
{

	jbyte* hdrPtr = (*env)->GetByteArrayElements(env, hdr, NULL);
	jsize nhdrbytes = (*env)->GetArrayLength(env, hdr);

	char *remoteip = (char *)(*env)->GetStringUTFChars(env, remoteIP, NULL);
	char *myip = (char *)(*env)->GetStringUTFChars(env, myIP, NULL);

	return(StartUDP((unsigned char *)hdrPtr, nhdrbytes, remoteip, remotePort, myip, myport, codec,  gain));
}

JNIEXPORT void JNICALL Java_PiJNI_RTLsdrJNI_stopUDP
(JNIEnv *env, jobject o)
{
	StopUDP();
}
