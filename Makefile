
all: client server test
buffer.o:
client: client.c
test: test.c
server: server.c buffer.o
	$(CC) buffer.o server.c  -o server

clean:
	rm server client socket buffer.o test
