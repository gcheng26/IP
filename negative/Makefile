# Makefile for negative example

# Object files
OBJS = negative.o main.o 

# Headers
HEADERS = negative.h

# Source files
SRC = $(OBJS:.o=.c) 

# Output file
OUT = negative

# Flags
CPPFLAGS = `pkg-config vips --cflags`
LDLIBS = `pkg-config vips --libs`
CFLAGS = -g -Wall -O3

$(OUT): $(OBJS)

.PHONEY: clean
clean:
	rm -f $(OBJS) $(OUT)
