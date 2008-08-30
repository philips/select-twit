#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "defines.h"

struct clients {
	fd_set set;
	int self;
	int max;
};

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

int client_add(struct clients *cli, int fd)
{
	int i;

	FD_SET(fd, &cli->set);

	if (fd > cli->max)
		cli->max = fd;

	return i;
}

void client_remove(struct clients *cli, int fd)
{
	int i;
	FD_CLR(fd, &cli->set);

	if (fd != cli->max)
		return;

	for (i = cli->max - 1; i >= 0; i--) {
		if (FD_ISSET(i, &cli->set)) {
			cli->max = i;
			return;
		}
	}
}

void client_init(struct clients *cli)
{
	FD_ZERO(&cli->set);
	cli->max = -1;
}

int client_accept(struct clients *cli, int server)
{
	int i, t, ret;
	struct sockaddr_un remote;

	t = sizeof(remote);
	ret = accept(server, (struct sockaddr *)&remote, &t);
	if (ret == -1) {
		perror("accept");
		exit(1);
	}
	client_add(cli, ret);

	printf("Connected %i.\n", ret, cli->max);

	return ret;
}

void spam_everyone(struct clients *cli, int exclude, char *str, int n)
{
	int i;

	for (i = 0; i <= cli->max; i++) {
		if (i == exclude || i == cli->self)
			continue;
		if (!FD_ISSET(i, &cli->set))
			continue;
		if (send(i, str, n, 0) < 0) {
			perror("send");
		}
	}
}


void client_handle_request(struct clients *cli, int fd)
{
	int n, t;
	char str[MSG_SIZE];
	char buf[MSG_SIZE - 10] = {0};

	n = recv(fd, buf, MSG_SIZE - 10, 0);
	if (n <= 0) {
		if (n < 0) perror("recv");
		client_remove(cli, fd);
		close(fd);
		return;
	}

	t = snprintf(str, MSG_SIZE, "%i: %s", fd, &buf);
	spam_everyone(cli, fd, str, t);
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
