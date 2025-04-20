CC = gcc
CFLAGS = -w

all: udpserver udpclient tcpserver tcpclient

udpserver: udpserver.c
	$(CC) $(CFLAGS) -o $@ $<

udpclient: udpclient.c
	$(CC) $(CFLAGS) -o $@ $<

tcpserver: tcpserver.c
	$(CC) $(CFLAGS) -o $@ $<

tcpclient: tcpclient.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f udpserver udpclient tcpserver tcpclient