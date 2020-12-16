NAME:=dwm-simple-status

SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

INCS = -I. -I/usr/include -I${X11INC}
LIBS = -L/usr/lib -lc -L${X11LIB} -lX11

CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS}
LDFLAGS = ${LIBS}
CC = cc

ifeq ($(LAPTOP),1)
	CFLAGS+=-DLAPTOP
endif

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJECTS): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -c $(SOURCES)

clean:
	rm -f $(OBJECTS)

.PHONY: clean
