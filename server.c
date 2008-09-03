#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>


#include "defines.h"

int debuglevel = 1;

int socket_setup(char *path)
{
	int s, len;
	struct sockaddr_un local;
	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, path);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);
	if (bind(s, (struct sockaddr *)&local, len) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(s, 5) == -1) {
		perror("listen");
		exit(1);
	}

	return s;
}

int client_add(struct clients *cls, int fd)
{
	int i;
	struct client *client = &cls->clients[fd];

	FD_SET(fd, &cls->set);

	if (fd > cls->max)
		cls->max = fd;

	client->fd = fd;
	client->msg = buffer_init();

	return i;
}

void client_remove(struct clients *cls, int fd)
{
	int i;
	struct client *client = &cls->clients[fd];

	FD_CLR(fd, &cls->set);

	buffer_free(client->msg);
	client->fd = -1;

	if (fd != cls->max)
		return;

	for (i = cls->max - 1; i >= 0; i--) {
		if (FD_ISSET(i, &cls->set)) {
			cls->max = i;
			return;
		}
	}
}

void client_init(struct clients *cls)
{
	FD_ZERO(&cls->set);
	cls->max = -1;
}

int client_accept(struct clients *cls, int server)
{
	int i, t, ret;
	struct sockaddr_un remote;

	t = sizeof(remote);
	ret = accept(server, (struct sockaddr *)&remote, &t);
	if (ret == -1) {
		perror("accept");
		exit(1);
	}
	client_add(cls, ret);

	debugprintf("Connected %i.\n", ret);

	return ret;
}

void spam_everyone(struct clients *cls, int exclude, char *str, int n)
{
	int i;

	for (i = 0; i <= cls->max; i++) {
		if (i == exclude || i == cls->self)
			continue;
		if (!FD_ISSET(i, &cls->set))
			continue;
		if (send(i, str, n, 0) < 0) {
			perror("send");
		}
	}
}

int client_handle_request(struct clients *cls, int fd)
{
	int n, t;
	int toread;

	struct buffer *buffer = cls->clients[fd].msg;
	char *str;


        if (ioctl(fd, FIONREAD, &toread)) {
		debugprintf("%s: FIONREAD\n", errno);
		return -1;
        }

	if (buffer->used + toread >= MAX_MSG_SIZE) {
		debugprintf("client sent %d bytes, too much data!\n", 
				buffer->used + toread);
		client_remove(cls, fd);
		close(fd);
		return -1;
	}

	buffer_grow(buffer, toread);

	n = recv(fd, buffer->buf + buffer->used, MAX_MSG_SIZE - buffer->used, 0);
	buffer->used += n;

	if (n <= 0) {
		if (n < 0) perror("recv");
		client_remove(cls, fd);
		close(fd);
		return;
	}

	if (buffer->buf[buffer->used - 1] != '\n')
		return 0;

	buffer->buf[buffer->used] = '\0';

	str = malloc(MAX_MSG_SIZE);
	if (!str)
		perror("malloc");

	t = snprintf(str, MAX_MSG_SIZE, "%i: %s", fd, buffer->buf);
	spam_everyone(cls, fd, str, t);
	buffer->used = 0;
	free(str);
}

int main(void)
{
	int s;
	int i;
	int ret;
        fd_set fds;
	struct clients cli;

	client_init(&cli);
	s = socket_setup(SOCK_PATH);
	cli.self = s;
	client_add(&cli, s);

	for (;;) {
		fds = cli.set;
		if (select(cli.max + 1, &fds, NULL, NULL, NULL) == -1) {
		    perror("select");
		    exit(1);
		}

		for (i = 0; i <= cli.max; i++) {
			if (!FD_ISSET(i, &fds))
				continue;

			if (i == cli.self) {
				client_accept(&cli, s);
				continue;
			}
			client_handle_request(&cli, i);
		}
	}

	return 0;
}
