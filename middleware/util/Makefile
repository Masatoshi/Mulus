INCDIR = ../include
LIBDIR = ../lib

CC = gcc
CFLAGS = -g -Wall -DDEBUG -I$(INCDIR)
LDFLAGS = 

OUT = libmls_util.a
SRCS = mls_log.c mls_config.c mls_net.c mls_evt.c mls_type.c
OBJS = $(SRCS:.c=.o)

.c.o:
	$(CC) -c $(CFLAGS) $*.c

all: install

install: $(OUT)
	mv $(OUT) $(LIBDIR)/

$(OUT): $(OBJS)
	ar r $(OUT) $(OBJS)

clean:
	rm -f $(OUT) $(OBJS)
