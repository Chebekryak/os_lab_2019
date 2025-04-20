CC = gcc
CFLAGS = -w -I.
LDFLAGS = -pthread
TARGETS = server client
LIBRARY = liblib.a
OBJS = lib.o

all: server client

$(LIBRARY): $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

server: server.c $(LIBRARY)
	$(CC) $(CFLAGS) $< -o $@ $(LIBRARY) $(LDFLAGS)

client: client.c $(LIBRARY)
	$(CC) $(CFLAGS) $< -o $@ $(LIBRARY) $(LDFLAGS)

clean:
	rm -f $(TARGETS) $(LIBRARY) $(OBJS)
