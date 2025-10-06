#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void dump_help(const char* exe_name) {
	const char* fmt = "<usage>: <%s> <port>, where port is the"
	                  " place you wanna let the client connect to!\n";
	printf(fmt, exe_name);
	return;
}

static inline int from_string_to_number(const char* port) {
	return atoi(port);
}

static const char* LOCAL_IP = "127.0.0.1";

int main(int argc, const char* argv[]) {
	if (argc != 2) {
		dump_help(argv[0]);
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(from_string_to_number(argv[1]));
	if (inet_pton(AF_INET, LOCAL_IP, &addr.sin_addr) < 0) {
		fprintf(stderr, "Error in processing the ip: %s", LOCAL_IP);
		return -1;
	}

	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_fd < 0) {
		fprintf(stderr, "Error in processing the socket");
		return -1;
	}

	if (connect(
	        client_fd,
	        (struct sockaddr*)&addr,
	        sizeof(struct sockaddr_in))
	    < 0) {
		fprintf(stderr, "Error in processing the connect");
		return -1;
	}

	printf("Successfully connected to server at %s:%s\n", LOCAL_IP, argv[1]);

	const char* message = "Hello from client";
	send(client_fd, message, strlen(message), 0);
	printf("Message sent to server\n");

	close(client_fd);

	return 0;
}