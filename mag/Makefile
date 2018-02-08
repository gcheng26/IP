

# Object files
OBJS = mag.o main.o 

# Headers
HEADERS = mag.h

# Source files
SRC = $(OBJS:.o=.c) 

# Output file
OUT = mag

# Flags
CPPFLAGS = `pkg-config vips --cflags`
LDLIBS = `pkg-config vips --libs` -lm

CFLAGS = -g -Wall -O3


$(OUT): $(OBJS)

.PHONEY: clean
clean:
	rm -f $(OBJS) $(OUT)
