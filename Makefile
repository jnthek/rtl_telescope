.PHONY: sdrshmbuffer.h

all: rtl2rbuff

rtl2rbuff: rtl2rbuff.c
	@echo "Compiling rtl2rbuff"
	gcc -O0 -Wall rtl2rbuff.c -o rtl2rbuff -lrt -lm -lpthread -lrtlsdr 

clean :
	rm -f rtl2rbuff 
