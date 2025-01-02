/*---------------------------------------------------------------------------
	Project:	      PiWxRx Weather receiver

	Module:		      Child process handler (Linux version)

	File Name:	      linux_spawn.c

	Author:		      Martin C. Alcock, VE6VH

	Revision:	      1.05

	Description:	Linux code only for creating a child process

					This program is free software: you can redistribute it and/or modify
					it under the terms of the GNU General Public License as published by
					the Free Software Foundation, either version 2 of the License, or
					(at your option) any later version, provided this copyright notice
					is included.

				  Copyright (c) 2018-2022 Praebius Communications Inc.

	Revision History:

---------------------------------------------------------------------------*/
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>

#include "rtl.h"

#define NUM_PIPES          2
 
#define PARENT_WRITE_PIPE  0
#define PARENT_READ_PIPE   1

#define READ_FD            0
#define WRITE_FD           1

#define CHILD_STDERR_WR    WRITE_FD
#define PARENT_STDERR_RD   READ_FD

#define PARENT_READ_FD  ( pipes[PARENT_READ_PIPE][READ_FD]   )
#define PARENT_WRITE_FD ( pipes[PARENT_WRITE_PIPE][WRITE_FD] )
 
#define CHILD_READ_FD   ( pipes[PARENT_WRITE_PIPE][READ_FD]  )
#define CHILD_WRITE_FD  ( pipes[PARENT_READ_PIPE][WRITE_FD]  )

pid_t child_proc;
int pipes[NUM_PIPES][NUM_PIPES];

// background process and structs to capture stderr from child
struct stderr_capture_t {
    BOOL        exit;
    int         stderr_pipe[NUM_PIPES];
    pthread_t   capture_fn;
} stderr_capture;

static void *capture_fn(void *arg)
{
    char buffer[256];
    ssize_t nRead;
    struct stderr_capture_t *s = arg;
    while(!s->exit) {
        int num_to_read = 256;
        if((nRead = read(s->stderr_pipe[PARENT_STDERR_RD], buffer, num_to_read)) > 0) {
            buffer[nRead] = '\0';
            fprintf(stderr,"%s",buffer);
        }
    }
}

// child process signal handler
void handle_sigchild(int sig)
{
    int saved_errno = errno;
    while (waitpid((pid_t)-1, 0, WNOHANG) > 0);

    fprintf(stderr, "Caught SIGCHILD: %s\n", geterrno(errno));
    errno = saved_errno;
}

// start the child process
BOOL initChildProcess(char *cmdline)
{
    // set up  a handler to catch a SIGCHILD
    struct sigaction sa;
    sa.sa_handler = &handle_sigchild;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if(sigaction(SIGCHLD, &sa, 0) < 0) {
        fprintf(stderr,"Sigaction for child failed\n");
        return FALSE;
    }
    
    // parse the command line
    int argc=0;
    char *argv[24], *tok;
    char *cmd = strdup(cmdline);

    while((tok=strsep(&cmd," ")) != NULL)	{
	argv[argc++] = tok;
    }
    argv[argc++] = NULL;

    // pipes for parent to write and read
    if(pipe(pipes[PARENT_READ_PIPE]) != -1) {
        if(pipe(pipes[PARENT_WRITE_PIPE]) == -1)    {
            fprintf(stderr,"Pipe Create failed\n");
            return FALSE;
        }
    }
    
    // setup the stderr pipe
    if(pipe(stderr_capture.stderr_pipe) == -1)  {
        fprintf(stderr, "Stderr capture pipe create failed\n");
        return FALSE;
    }
    
    // for..
    child_proc=fork();
    if(child_proc == -1)    {
        fprintf(stderr,"Fork failed\n");
        return FALSE;
    }
    if(child_proc == 0) {
        // redirect stderr before anything else
        int dupestat = dup2(stderr_capture.stderr_pipe[CHILD_STDERR_WR], STDERR_FILENO);
        if(dupestat < 0)    {
            fprintf(stderr, "Stderr Pipe failure\n");
            exit(0);
        }        
        // create a pipe to write
        int dup1stat = dup2(CHILD_READ_FD, STDIN_FILENO);
        int dup2stat = dup2(CHILD_WRITE_FD, STDOUT_FILENO);
        if((dup1stat < 0) || (dup2stat == 0))	{
            fprintf(stderr,"Pipe dup failure %d:%d\n", dup1stat, dup2stat);
            exit(0);
        } else {
            if(dup1stat != STDIN_FILENO)
                fprintf(stderr,"Pipe create (in) returned: %d\n", dup1stat);
            if(dup2stat != STDOUT_FILENO)
                fprintf(stderr,"Pipe create (out) returned: %d\n", dup1stat);
        }
    
        close(PARENT_READ_FD);
        close(PARENT_WRITE_FD);

        // start the processs
        int execstat = execv(argv[0], argv);
        // only returns if an error occurred
        if(execstat < 0)
            fprintf(stderr, "execv returned: %s\n", geterrno(errno));
        exit(0);
        
    } else {
        // parent process: start the stderr capture
        stderr_capture.exit = FALSE;
        pthread_create(&stderr_capture.capture_fn, NULL, capture_fn, (void *)(&stderr_capture));
        
        close(CHILD_READ_FD);
        close(CHILD_WRITE_FD);
    }
    return TRUE;  
}

void CloseChildProcess()
{
    DEBUGPRINTF("Shutting Down..\n");
    stderr_capture.exit = TRUE;
    
    close(CHILD_WRITE_FD);
    close(PARENT_READ_FD);
    
    kill(child_proc, SIGTERM);
    sleep(2);
    kill(child_proc, SIGKILL);
}

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data.  
int ReadFromPipe(RTL_SAMPLE *buffer, int bfrsiz) 
{ 
	ssize_t dwRead; 
	size_t num_to_read = bfrsiz*sizeof(RTL_SAMPLE);

	DEBUGLEVEL(DEBUG_MSGS)
	 fprintf(stderr, "Reading %d samples from pipe\n", bfrsiz);

	dwRead = read(PARENT_READ_FD, buffer, num_to_read);

	if(dwRead <= 0)
		return dwRead;
	else 
		return (dwRead/sizeof(RTL_SAMPLE));
}
