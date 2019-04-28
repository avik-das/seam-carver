CFLAGS = -g -Wall -pedantic
LDLIBS = -lm

seam-carver: seam-carver.o
seam-carver.o: seam-carver.c

.PHONY: clean
clean:
	rm seam-carver.o seam-carver
