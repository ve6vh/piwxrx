.SUFFIXES: 
.SUFFIXES: .c .o

CC=cc
COMMON=Common
SRCDIR=Source
INCDIR=Include
OBJDIR=bin
LIBDIR=../lib
JNILIB = $(OBJDIR)/libpiwxrx.so
OBJLIB = $(OBJDIR)/librtl.a
TARLIB = piwxrx-libs.tar.gz

LFLAGS1 = -pthread -L$(OBJDIR) -lrtl
LFLAGS2 = -lm -lrt -lpthread -Wl,--unresolved-symbols=report-all
USBFLAGS = -lasound -lusb-1.0
CFLAGS= -I$(INCDIR) -pthread -fPIC -g

SRC = $(shell find $(COMMON) -name '*.c')
LIBOBJ = $(patsubst $(COMMON)/%.c,$(OBJDIR)/%.o,$(SRC))

all:	$(JNILIB) filereader

filereader: $(OBJLIB) $(OBJDIR)/filereader.o $(OBJDIR)/usb.o
	$(CC) $(CFLAGS) $(OBJDIR)/filereader.o $(OBJDIR)/rtl.o $(OBJDIR)/usb.o $(LFLAGS1) -o filereader $(USBFLAGS) $(LFLAGS2)
        cp filereader ../local

$(OBJDIR)/filereader.o: $(SRCDIR)/filereader.c $(SRCDIR)/rtl.c $(SRCDIR)/usb.o
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/filereader.o $(SRCDIR)/filereader.c

$(OBJDIR)/usb.o: $(SRCDIR)/usb.c
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/usb.o $(SRCDIR)/usb.c

$(OBJDIR)/rtl.o: $(SRCDIR)/rtl.c
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/usb.o $(SRCDIR)/rtl.c

$(JNILIB): $(OBJLIB)
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/rtl.o $(SRCDIR)/rtl.c
	cc -shared -o $(JNILIB) $(LIBOBJ) $(OBJDIR)/rtl.o $(LFLAGS2) 

$(OBJLIB):	$(LIBOBJ)
	ar -cqs $(OBJLIB) $(LIBOBJ)

$(OBJDIR)/%.o: $(COMMON)/%.c
	$(CC) $(CFLAGS) -c -o $(OBJDIR)/$*.o $(COMMON)/$*.c

clean:
	rm -r $(OBJDIR)
	mkdir $(OBJDIR)
	touch Source/*

install: $(JNILIB)
	sudo cp $(JNILIB) $(LIBDIR)

tar:  $(JNILIB)
	rm $(TARLIB)
	tar -zcvf $(TARLIB) $(JNILIB) $(LIBDIR)/*.jar
