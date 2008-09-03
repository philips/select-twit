#include <stdio.h>
#include <assert.h>
#define SOCK_PATH "./socket"
#define MAX_MSG_SIZE 150

#define BUF_CHUNK 1024

extern int debuglevel;

#define debugprintf(fmt, ...)\
if (debuglevel) fprintf(stderr, __FILE__ ":%d: " fmt, __LINE__,  __VA_ARGS__);

struct buffer {
	char *buf;
	int used;
	int size;
};


struct client {
	int fd;
	/* if a message isn't completed by timeout close the connection */
	time_t timeout;
	struct buffer *msg;
};

struct clients {
	fd_set set;

	/* TODO: Make clients dynamic and stuff */
	struct client clients[32];
	int used;
	int max;

	int self;
};


struct buffer *buffer_init(void);
void buffer_free(struct buffer *b);
int buffer_grow(struct buffer *b, size_t size);
