
SRCS = main.c memlib.c pagemap.c chaos.c
HEADERS = memlib.h pagemap.h chaos.h

main: $(SRCS) $(HEADERS)
	gcc -O2 -Wall -o main $(SRCS)

clean:
	rm -f main *~
