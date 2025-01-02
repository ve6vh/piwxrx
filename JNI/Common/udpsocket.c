/*---------------------------------------------------------------------------
	Project:	      PiWxRx Weather receiver

	Module:		      UDP socket code

	File Name:		  udpsocket.c

	Author:		      Martin C. Alcock, VE6VH

	Revision:	      1.05

	Description:	  Contains the code to open, close and send on a UDP socket
					  
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
#include <winsock2.h>
#include <ws2tcpip.h>

WSADATA wsaData;

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>

#define	SOCKET_ERROR	-1
typedef int		SOCKET;
#endif

#include <stdio.h>
#include "rtl.h"

SOCKET datagram;
struct sockaddr_in remote_addr;
struct sockaddr_in my_addr;

// return platform dependent error
int PrintErr(void)
{
#ifdef _WIN32      
			return(WSAGetLastError());
#else
			return(errno);
#endif   
}

/*---------------------------------------------------------------------------

	FUNCTION:	OpenSocket

	INPUTS:		remoteIP, remoteport, myIP, my port

	OUTPUTS:	UDP Socket created, TRUE if successful, FALSE otherwise

---------------------------------------------------------------------------*/
BOOL OpenSocket(char *remoteip, int remoteport, char *myip, int myport)
{

#ifdef _WIN32    
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		DEBUGLEVEL(DEBUG_UDP)
			fprintf(stderr, "Socket error %d\n", WSAGetLastError());
		return FALSE;
	}
#endif

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(myport);
	inet_pton(AF_INET, myip, &my_addr.sin_addr.s_addr);

	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(remoteport);
	inet_pton(AF_INET, remoteip, &remote_addr.sin_addr.s_addr);

	if ((datagram = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		DEBUGLEVEL(DEBUG_UDP)
			fprintf(stderr, "Socket error %d\n", PrintErr());
		return FALSE;
	}

	if(bind(datagram, (const struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		DEBUGLEVEL(DEBUG_UDP)
			fprintf(stderr, "Bind failed %d\n", PrintErr());
		return FALSE;
	}

	DEBUGPRINTF("Open Socket passed\n");

	return TRUE;
}

/*---------------------------------------------------------------------------

	FUNCTION:	    Send

	INPUTS:		    buffer, length

	OUTPUTS:	    Datgagram, TRUE if successful, FALSE otherwise

	DESCRIPTION:	send a datagram on the open socket

---------------------------------------------------------------------------*/
BOOL Send(const char *buffer, int nbytes)
{
	if (sendto(datagram, (const char *)buffer, nbytes, 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr))
		!= SOCKET_ERROR)
		return TRUE;
    
	DEBUGLEVEL(DEBUG_UDP)
		fprintf(stderr, "Send failed %d\n", PrintErr());

	  return FALSE;
}

/*---------------------------------------------------------------------------

	FUNCTION:	    CloseSocket

	INPUTS:		    none

	OUTPUTS:	    none

	DESCRIPTION:	close the socket

---------------------------------------------------------------------------*/
void CloseSocket(void)
{
#ifdef _WIN32
	closesocket(datagram);
#else
	close(datagram);
#endif
}
