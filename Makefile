
all: client server
echoc: client.c
echos: server.c

clean:
	rm server client socket
