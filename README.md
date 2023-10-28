PiWxRx is a NOAA weather radio receiver that can recieve audio and digital data from a radio source.

It implements a SIP extension to an Asterisk PBX, when called it auto-answers after two rings
and forwards the received audio using the RTP protocol. The audio source can be from an RTL-SDR dongle, or any ALSA
USB based audio device. For test purposes it can read from a file instead.

It also implements a demodulator for NOAA messages; and interprets them using a built-in JSON database. There
are several modes for delivering the decoded message from dumping it to a log file, e-mailing, or posting to a 
website  using an http get, or the SOAP protcol.

It comes preconfigured to dump messages to the console from a disc file for ease of testing, and to run on an ARM-based
device. It is recommended to be run as a linux service.

The upper layers are written in Java, and require the OpenJDK-11 run time enviromnent to operate. On slower systems such
as a Raspberry Pi or other devices that use an SD card as it main disk, it is recommended that the expanded class file hierarchy
be used instead of a jar file. For systems with a faster hard drive, the jar file can be used. The code, by definition, is portable.

For the raspberry Pi, all files are contained in the archive PiWxRx.tar.gz. Download and unzip it, copy the files to /etc/PiWxRx
and start it up with the runpiwx.stdout. It comes preconfigured to decode a message in a disk file. If you see the decoded message,
all is well. Follow the instructions in the manual to complete the installation.

The lower level (time critical) layers are written in C, and can be ported to any target system by using the appropriate
C compiler. The source code of the JNI (Java Native Interface) layer is supplied in the JNI directory. The precompiled file
is for ARM architecture devices only, it can be recompiled for any custom platform.

To customize for a specific application, consult the documentation supplied. The current revision level is 4.4.2. The PiWxRX.jar
file can be used to update an existing version to the latest revision.

The tarball PiWxRx86.tar.gz is a self-contained implementation for X86 linux systems. Simply unzip it, customize the xml
file and it is ready to go. As of revision 4.4.1, more than one instance can be run on the same machine, consult the
documentation for more details. It is intended to be installed in /etc/PiWxRx.

A problem was encountered with an incompatibility when using JDK11, in the email mode due to libraries that had been
deprecated. This problem has been rectified by using a different JAR file, PiWxRx86_JDK11.tar.gz. Installation is the same
as the original.

There are two files used to run it, runpiwx and runpiwx.stdout. The former writes any errors to a file called NOAA.log, 
including a decoded messages in the 'dump' mode. The latter will write everything to the console, and is useful in setup
and debugging.

PiWxRxWeb is an extension to the Pi receiver to service mutliple receivers. It runs under Apache Tomcat 9 as a website,
where receivers can post NOAA messages to an SQL database. Subscribers can log in to their own account and select which
messages are appropriate for them from the database. Decoded messages are e-mailed to them via an external e-mail server,
and may also be sent to a SIP phone using an e-mail to SIP portal. The portal attaches to a dedicated extension on an
asterisk server, which must be enabled for SIP messaging. The portal and destination phone must be on the same server.

For support, e-mail ve6vh@ve6vh.org.
