#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define MSG_SZ (30)

static void dump_help(const char* exe_name) {
	const char* fmt = "<usage>: <%s> <port>, where port is the"
	                  " place you wanna let the client connect to!\n";
	printf(fmt, exe_name);
	return;
}

static inline int from_string_to_number(const char* port) {
	return atoi(port);
}

static inline int min_one(int a, int b) {
	return a > b ? b : a;
}

int main(int argc, const char* argv[]) {
	if (argc != 2) {
		dump_help(argv[0]);
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(from_string_to_number(argv[1]));
	addr.sin_addr.s_addr = INADDR_ANY;

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_fd <= 0) {
		fprintf(stderr, "Error in processing the socket");
		return -1;
	}

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "Bind Error! Do the port already be registered?");
		return -1;
	}

	listen(server_fd, 10);
	socklen_t szOfClientfd = sizeof(struct sockaddr_in);
	struct sockaddr_in client_addr;
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &szOfClientfd);
	char message[MSG_SZ];
	int bytes_read = read(client_fd, message, MSG_SZ);
	message[min_one(bytes_read + 1, MSG_SZ)] = '\0';

	printf("Read from clients: %s\n", message);

	close(client_fd);
	close(server_fd);
}