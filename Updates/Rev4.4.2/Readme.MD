This update is an alpha release of the email body templating. It has not been extensively tested.
To install, first backup your original files. Then rename the current bin directory to something
different, and expand the tarball piwxrxbin.tar.gz, it will create a new bin folder and subsequent
class folder. Do a visual check to ensure that the directory structure is the same as the old version. 
No new class files have been added.

To install, text edit the PiWxRx.xml file to suit as usual. Add a line to the <forwarding> stanza to
include the line 	bodytemplate="emailbody.xml" after the subject line prototype. The current template is
ready to go and mimics the existing e-mail format. If the bodytemplate entry is missing, the default format
will be used.

The template entries are:
<Header> The first line of the message
<Detail> Repeated for every same code
<Trailer>At the end of the message
<PostScript>A extra line at end of the email message, for a disclaimer, etc. Can be omitted.

Run a usual.
