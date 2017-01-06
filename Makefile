CFLAGS= -g -O2 -Wall
all: ddns.c base64.o
	$(CC) $(CFLAGS) $^ -o ddns

base64.o: base64.c
	$(CC) $(CFLAGS) -c $^

install:
	sh install-sh
clean:
	@rm -f *.o
	@rm ddns
