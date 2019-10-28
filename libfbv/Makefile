
CC ?= gcc
CFLAGS ?= -g

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

testfbv: testfbv.o fbv.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf testfbv *.o

.PHONY: clean
