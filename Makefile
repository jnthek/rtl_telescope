.PHONY: sdrbuffer.h

all: rtl2binary stdout2shmbuf

rtl2binary: rtl2binary.c
	@echo "Compiling rtl2binary"
	gcc -O0 -Wall rtl2binary.c -o rtl2binary -lrt -lrtlsdr

stdout2shmbuf: stdout2shmbuf.c
	@echo "Compiling pipe to shm tools"
	gcc -Wall stdout2shmbuf.c -o stdout2shmbuf -lm -lrt -lpthread

clean :
	rm -f rtl2binary stdout2shmbuf
