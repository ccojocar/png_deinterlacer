CC=gcc
CFLAGS=-O2 -Wall -std=c99
LDFLAGS=-L/usr/local/lib -lpng15
SRCDIR=.
SRC=$(foreach dir,$(SRCDIR),$(wildcard $(dir)/*.c))
OBJS=$(SRC:.c=.o)
BIN=png_deinterlacer

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@
%.o: %.c
	$(CC) $(CFLAGS) -c $<
clean:
	rm -rf $(BIN) *.o

