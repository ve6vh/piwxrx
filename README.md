PiWxRx is a NOAA weather radio receiver that can recieve audio and digital data from a radio source.

It implements a SIP extension to an Asterisk PBX, when called it auto-answers after two rings
and forwards the received audio using the RTP protocol. The audio source can be from an RTL-SDR dongle, or any ALSA
USB based audio device. For test purposes it can read from a file instead.

It also implements a demodulator for NOAA messages; and interprets them using a built-in JSON database. There
are several modes for delivering the decoded message from dumping it to a log file, e-mailing, or posting to a 
website  using an http get, or the SOAP protcol.

The archives in this repository are for linux machines only such as the Raspberry Pi, X86 based machines and the Odroid C4. It
is preconfigured to dump messages to the console from a disc file for ease of testing.

It is intended to be installed in /etc/PiWxRx, so after downloading and expanding the archive, copy all the files there.

The current revision level is 4.4.2, older versions can be upgraded.

Pick ONE of the following four archives:

PiWxRx.tar.gz is for the raspberry Pi only and is at 4.4.2 already. Download and unzip it, copy the files to the recommended directory
and run it. If you see the  decoded message, on the screnn, all is well. Follow the instructions in the manual to complete the installation.

PiWxRx86.tar.gz is a self-contained implementation for X86 linux systems. As of revision 4.4.1, more than one instance 
can be run on the same machine, consult the documentation for more details. An upgrade to 4.4.2 is contained in PiWxRx.jar file.

PiWxRx86_JDK11.tar.gz is a later version of 4.4.1 for x86 machines for JDK11. problem was encountered with an incompatibility 
in the email mode due to libraries that had been deprecated. Installation is the same as the original, however also replace the file
PiWxRx.jar with the latest in this repo for 4.4.2.

PiWxRx_Odroid.tar.gz is a version of 4.4.1 for the Odroid C4, a 64-bit version of the raspberry Pi. The archive is at 4.4.1, add the
upgrade file for 4.4.2.

Once you have installed the correct version, do NOT modify anyything unti you have run the code using runpiwx.stdout. You should see
the decoded message on the console. After that, you can try customizing the forwarding, but stil using the same disk file source,
and see if you get an email message. If you do, then you can set up the audio source, and modify the JSON database for your local area.

Other platforms
===============
A custom version can be made for a different processor by recompiling the native code in the JNI library. Do not attempt
unless you are familiar with developing C code for the system of choice. The Java code will run as it is portable by definition.

PIWxRXWeb
=========
PiWxRxWeb is an extension to the Pi receiver to service mutliple receivers. It runs under Apache Tomcat 9 as a website,
where receivers can post NOAA messages to an SQL database. Subscribers can log in to their own account and select which
messages are appropriate for them from the database. Decoded messages are e-mailed to them via an external e-mail server,
and may also be sent to a SIP phone using an e-mail to SIP portal. The portal attaches to a dedicated extension on an
asterisk server, which must be enabled for SIP messaging. The portal and destination phone must be on the same server.

For support, e-mail ve6vh@ve6vh.org.
