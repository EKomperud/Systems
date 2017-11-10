REDACT = redact.c
RACKET = racket

TEST_INPUTS = ex1.so ex2.so

all: redact call $(TEST_INPUTS)

redact: $(REDACT) decode.c decode.h
	gcc -Wall -O2 -o redact $(REDACT) decode.c

call: call.c
	gcc -Wall -O2 -o call call.c -ldl

%.so: %.c
	gcc -shared -O2 -fPIC -o $@ $<


test: redact call $(TEST_INPUTS)
	for i in $(TEST_INPUTS) ; do  \
          $(RACKET) test.rkt $$i; \
        done

clean:
	rm -f redact call *.so
