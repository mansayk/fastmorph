#############################################################################
#
# Makefile for building: fastmorph & fastngrams
#
#############################################################################

CC         = gcc
DEFINES    = -DJSMN_PARENT_LINKS=1
	     #-DJSMN_PARENT_LINKS=1 Activating fastest mode in JSMN JSON library
CFLAGS     = -g \
	     -Wall \
	     -Wextra \
	     -std=c99 \
	     -pthread \
	     -Ofast \
	     -fstack-protector \
	     -D_FORTIFY_SOURCE=2 \
	     -mcmodel=large
	     #-march=bdver1 # For AMD FX-4100 processor
INCPATH    = -I mysql-connector-c/include
LIBS       = -L/usr/lib64/mysql -l mysqlclient
SOURCES    = b64/decode.c \
	     point_of_time.c
FASTMORPH  = fastmorph
FASTNGRAMS = fastngrams
CLEAR      = clear
RM         = rm -f

all: $(FASTMORPH) $(FASTNGRAMS)

$(FASTMORPH): $(FASTMORPH).c clean
	$(CLEAR)
	$(CC) $(CFLAGS) $(INCPATH) $(LIBS) -o $(FASTMORPH) $(FASTMORPH).c $(SOURCES)
#	./$(FASTMORPH)

$(FASTNGRAMS): $(FASTNGRAMS).c clean
	$(CLEAR)
	$(CC) $(CFLAGS) $(INCPATH) $(LIBS) -o $(FASTNGRAMS) $(FASTNGRAMS).c $(SOURCES)
	./$(FASTNGRAMS)

clean:
	$(RM) $(FASTMORPH) *~ .*~ /tmp/fastmorph.socket
	$(RM) $(FASTNGRAMS) *~ .*~ /tmp/fastngrams.socket

