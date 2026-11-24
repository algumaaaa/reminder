CC = gcc
CFLAGS = -Wall -Wextra -lm -I./include/ 
CFLAGS += $(shell pkg-config --cflags libnotify libpipewire-0.3)
LDFLAGS = $(shell pkg-config --libs libnotify libpipewire-0.3)
SOURCES = ./src/*.c

all: reminder #run clean

reminder:
	$(CC) $(SOURCES) $(CFLAGS) -o reminder $(LDFLAGS)

run:
	./reminder

clean:
	rm reminder
