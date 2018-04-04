# Object files
OBJS = Sobel.o main.o 

# Headers
HEADERS = Sobel.h

# Source files
SRC = $(OBJS:.o=.c) 

# Output file
OUT = Sobel

# Flags
CPPFLAGS = `pkg-config vips --cflags`
LDLIBS = `pkg-config vips --libs` -lm

CFLAGS = -g -Wall -O3


$(OUT): $(OBJS)

.PHONEY: clean
clean:
	rm -f $(OBJS) $(OUT)
