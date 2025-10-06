#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

// --- 宏定义 ---
#define PORT 12345
#define MAX_EVENTS 64
#define LISTEN_BACKLOG 128
#define BUF_SIZE 4096

// 错误检查宏
#define CHECK(expr, msg)    \
	if (expr) {             \
		perror(msg);        \
		exit(EXIT_FAILURE); \
	}

// --- 辅助函数 ---

/**
 * @brief 设置文件描述符为非阻塞模式
 * @param fd 文件描述符
 * @return 0 成功，-1 失败
 */
static int set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		perror("fcntl(F_GETFL)");
		return -1;
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		perror("fcntl(F_SETFL)");
		return -1;
	}
	return 0;
}

/**
 * @brief 处理监听套接字的连接事件
 * @param epfd epoll 文件描述符
 * @param listen_fd 监听套接字
 */
static void handle_new_connection(int epfd, int listen_fd) {
	// 循环 accept 以应对 ET 模式下可能同时到达的多个连接
	while (1) {
		struct sockaddr_in cli_addr;
		socklen_t cli_len = sizeof(cli_addr);
		int conn_fd = accept(listen_fd, (struct sockaddr*)&cli_addr, &cli_len);

		if (conn_fd == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// 所有传入连接已处理完毕
				break;
			} else if (errno == EINTR) {
				// 被信号中断，继续尝试
				continue;
			} else {
				perror("accept");
				break;
			}
		}

		if (set_nonblocking(conn_fd) == -1) {
			close(conn_fd);
			continue;
		}

		struct epoll_event ev;
		// 客户端连接使用 EPOLLIN 和 EPOLLET (边缘触发)
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = conn_fd;

		if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev) == -1) {
			perror("epoll_ctl: conn_fd");
			close(conn_fd);
		} else {
			char ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &cli_addr.sin_addr, ip, sizeof(ip));
			printf("Accepted %s:%d -> fd %d\n", ip, ntohs(cli_addr.sin_port), conn_fd);
		}
	}
}

/**
 * @brief 处理客户端套接字的可读事件（以及错误/挂断）
 * @param epfd epoll 文件描述符
 * @param fd 客户端套接字
 * @param evs 就绪事件掩码
 */
static void handle_client_data(int epfd, int fd, uint32_t evs) {
	// 错误或挂断事件，关闭连接
	if (evs & (EPOLLERR | EPOLLHUP)) {
		printf("Client fd %d error/hangup. Closing.\n", fd);
		close(fd);
		return;
	}

	// 可读事件 (ET 模式：必须循环读，直到 EAGAIN/EWOULDBLOCK)
	if (evs & EPOLLIN) {
		char buf[BUF_SIZE];
		ssize_t total_read = 0;
		int closed = 0;

		while (1) {
			ssize_t count = read(fd, buf + total_read, sizeof(buf) - total_read);

			if (count == -1) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					// 没有更多数据，正常退出循环
					break;
				} else if (errno == EINTR) {
					// 被信号中断，继续
					continue;
				} else {
					// 其他读取错误
					perror("read");
					closed = 1;
					break;
				}
			} else if (count == 0) {
				// 对端关闭连接
				printf("Client fd %d closed by peer\n", fd);
				closed = 1;
				break;
			}

			total_read += count;

			// 简单处理：如果缓冲区已满，先处理/回显当前数据，然后继续尝试读
			// 生产环境中，此处应是更复杂的逻辑
			if (total_read >= BUF_SIZE) {
				break; // 避免 over-read 导致的缓冲区溢出（虽然 read 本身会限制，但避免逻辑复杂化）
			}
		}

		if (closed) {
			close(fd);
			return;
		}

		// --- 数据回显逻辑 ---
		if (total_read > 0) {
			ssize_t written = 0;
			// 循环写，直到写完或遇到 EAGAIN
			while (written < total_read) {
				ssize_t w = write(fd, buf + written, total_read - written);

				if (w == -1) {
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						// 发送缓冲区已满：理论上应将未发送数据存入一个写缓冲队列，
						// 并注册 EPOLLOUT 事件等待下次写就绪。
						// 这里仅作简单提示和 EPOLLOUT 注册。
						fprintf(stderr, "Warning: Partial write for fd %d. Data not handled: %zd bytes\n",
						        fd, total_read - written);

						struct epoll_event mod_ev;
						mod_ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
						mod_ev.data.fd = fd;
						epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &mod_ev);
						break;
					} else if (errno == EINTR) {
						continue;
					} else {
						perror("write");
						close(fd);
						return;
					}
				} else {
					written += w;
				}
			}
		}
	}

	// 处理 EPOLLOUT 事件：仅在上次写入未完成时才会被关注
	if (evs & EPOLLOUT) {
		// 在实际应用中，此处应处理并发送写缓冲队列中剩余的数据。
		// 完成发送后，需要取消对 EPOLLOUT 的关注，以避免不必要的唤醒。

		// 假设数据已全部发送或暂时不发送：移除 EPOLLOUT
		struct epoll_event mod_ev;
		mod_ev.events = EPOLLIN | EPOLLET; // 重新只关注可读
		mod_ev.data.fd = fd;
		if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &mod_ev) == -1) {
			perror("epoll_ctl(MOD/remove EPOLLOUT)");
		}
		// 如果数据未发完，则不应移除 EPOLLOUT，直到队列清空
	}
}

// --- 主函数 ---

int main() {
	int listen_fd, epfd;
	struct sockaddr_in addr;

	// 1. 创建监听套接字
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	CHECK(listen_fd == -1, "socket");

	// 设置 SO_REUSEADDR 避免 TIME_WAIT 导致重启失败
	int on = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
		perror("setsockopt(SO_REUSEADDR)");
	}

	// 2. 绑定地址
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	CHECK(bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1, "bind");

	// 3. 设置非阻塞
	CHECK(set_nonblocking(listen_fd) == -1, "set_nonblocking(listen_fd)");

	// 4. 监听
	CHECK(listen(listen_fd, LISTEN_BACKLOG) == -1, "listen");

	// 5. 创建 epoll 实例
	epfd = epoll_create1(EPOLL_CLOEXEC); // 推荐使用 EPOLL_CLOEXEC
	CHECK(epfd == -1, "epoll_create1");

	struct epoll_event ev, events[MAX_EVENTS];

	// 6. 注册监听套接字到 epoll
	ev.events = EPOLLIN | EPOLLET; // 监听可读（新连接），使用 ET 模式
	ev.data.fd = listen_fd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
		perror("epoll_ctl: listen_fd");
		close(epfd);
		close(listen_fd);
		exit(EXIT_FAILURE);
	}

	printf("Epoll Echo Server: Listening on port %d\n", PORT);

	// 7. 主事件循环
	while (1) {
		int n = epoll_wait(epfd, events, MAX_EVENTS, -1);

		if (n == -1) {
			if (errno == EINTR) {
				continue; // 被信号中断
			}
			perror("epoll_wait");
			break; // 致命错误，退出循环
		}

		for (int i = 0; i < n; ++i) {
			int fd = events[i].data.fd;
			uint32_t evs = events[i].events;

			if (fd == listen_fd) {
				handle_new_connection(epfd, listen_fd);
			} else {
				handle_client_data(epfd, fd, evs);
			}
		}
	}

	// 清理资源
	printf("Server shutting down...\n");
	close(listen_fd);
	close(epfd);
	return 0;
}