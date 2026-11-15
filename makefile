CC = gcc
CFLAGS = -Wall -Wextra -lm -I./include/ 
CFLAGS += $(shell pkg-config --cflags libnotify)
LDFLAGS = $(shell pkg-config --libs libnotify)
SOURCES = ./src/*.c

all: reminder #run clean

reminder:
	$(CC) $(SOURCES) $(CFLAGS) -o reminder $(LDFLAGS)

run:
	./reminder 16:19

clean:
	rm reminder
