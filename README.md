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

The current suppported revision level is 4.4.2, code can be found in that directory.

How To Install
==============
Copy all the files from the etc directory to /etc/PiWxRx, add the appropriate object code from the repo. Before making
any configuration changes, run the file 'piwxrx.stdout'. If it is successful, you should see a decoded tornado watch
on the console. Consult the documentation on how to configure the xml file going forward.

PIWxRXWeb
=========
PiWxRxWeb is an extension to the Pi receiver to service mutliple receivers. It runs under Apache Tomcat 9 as a website,
where receivers can post NOAA messages to an SQL database. Subscribers can log in to their own account and select which
messages are appropriate for them from the database. Decoded messages are e-mailed to them via an external e-mail server,
and may also be sent to a SIP phone using an e-mail to SIP portal. The portal attaches to a dedicated extension on an
asterisk server, which must be enabled for SIP messaging. The portal and destination phone must be on the same server.

For support, e-mail ve6vh@ve6vh.org.
