# Object files
OBJS = Canny.o main.o 

# Headers
HEADERS = Canny.h

# Source files
SRC = $(OBJS:.o=.c) 

# Output file
OUT = Canny

# Flags
CPPFLAGS = `pkg-config vips --cflags`
LDLIBS = `pkg-config vips --libs` -lm

CFLAGS = -g -Wall -O3


$(OUT): $(OBJS)

.PHONEY: clean
clean:
	rm -f $(OBJS) $(OUT)
