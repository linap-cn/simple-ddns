CFLAGS= -O2 -Wall -s
all: oray.c base64.o
	$(CC) $(CFLAGS) $^ -o oray

base64.o: base64.c
	$(CC) $(CFLAGS) -c $^

install:
	sh install-sh
clean:
	@rm -f *.o
	@rm oray