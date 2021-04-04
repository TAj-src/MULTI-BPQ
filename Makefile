#
# Makfile for G0LGS's multi server
#

VERS=0.02
VDATE=(04 April 2021)

CFLAGS = -Wall -O2 -g -DVersion='"$(VERS)"' -DVdate='"$(VDATE)"'

all: multi

CC = gcc
LD = gcc
LIBS = 

# Static
LDFLAGS = -static-libgcc
# Dynamic
#LDFLAGS =

.c.o:
		$(CC) $(CFLAGS) -c $<

multi:		Makefile multi.o utils.o
		$(LD) $(LDFLAGS) -o multi multi.o utils.o $(LIBS)

multi.o:	Makefile multi.c

utils.o:	Makefile utils.c utils.h

clean:
		rm -f core multi *.o *.~*


