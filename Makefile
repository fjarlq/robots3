DEFS    = -DUSE_FLOCK
CFLAGS  = $(DEFS) -O2 -Wall -Wextra
LDFLAGS = -lncurses
HEADERS = robots.h
SRC     = good.c main.c opt.c robot.c score.c user.c
OBJS    = $(SRC:c=o)
BIN     = robots3
MAN     = $(BIN).6
BINDIR  = /usr/games
MANDIR  = /usr/share/man/man6

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

install: $(BIN) $(MAN)
	install $(BIN) $(BINDIR)
	install $(MAN) $(MANDIR)

clean:
	$(RM) $(BIN) $(OBJS)

$(OBJS): $(HEADERS)

.PHONY: all install clean
