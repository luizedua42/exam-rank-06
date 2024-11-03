#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct s_client{
	int id;
	char msg[300000];
} t_client;
t_client clients[1024];

fd_set read_set, write_set, current;
int max_fd = 0, global_id = 0;
char send_buffer[400000], recv_buffer[300000];

void err(char *msg){
	if (msg)
		write(2, msg, strlen(msg));
	else
		write(2, "Fatal error", 11);
	write(2, "\n", 1);
	exit(1);
}

void ft_send(int e) {
	for (int fd = 0; fd <= max_fd; fd++) {
		if (FD_ISSET(fd, &write_set) && fd != e) {
			if (send(fd, send_buffer, strlen(send_buffer), 0) == -1)
				err(NULL);
		}
	}
}

int main (int argc, char **argv){
	if(argc != 2)
		err("Wrong number of arguments");

	struct sockaddr_in serveraddr;
	socklen_t len;
	int serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverfd == -1)
		err(NULL);
	max_fd = serverfd;

	FD_ZERO(&current);
	FD_SET(serverfd, &current);
	bzero(clients, sizeof(clients));
	bzero(&serveraddr, sizeof(serveraddr));
	
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(atoi(argv[1]));

	if (bind(serverfd, (const struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1 || listen(serverfd, 100) == -1)
		err(NULL);

	while (1) {
		read_set = write_set = current;
		if(select(max_fd + 1, &read_set, &write_set, NULL, NULL) == -1) continue;
		for (int fd = 0; fd <= max_fd; fd++) {
			if(FD_ISSET(fd, &read_set)){
				if (fd == serverfd) {
					int clientfd = accept(serverfd, (struct sockaddr *)&serveraddr, &len);
					if (clientfd == -1) continue;
					if(clientfd > max_fd) max_fd = clientfd;
					clients[clientfd].id = global_id++;
					FD_SET(clientfd, &current);
					sprintf(send_buffer, "server: client %d has just joined\n", clients[clientfd].id);
					ft_send(clientfd);
				}
				else {
					int ret = recv(fd, recv_buffer, sizeof(recv_buffer), 0);
					if(ret <= 0) {
						sprintf(send_buffer, "server: client %d just left\n", clients[fd].id);
						ft_send(fd);
						FD_CLR(fd, &current);
						close(fd);
						bzero(clients[fd].msg, sizeof(clients[fd].msg));
					}
					else {
						for (int i = 0, j = strlen(clients[fd].msg); i < ret; i++, j++) {
							clients[fd].msg[j] = recv_buffer[i];
							if(clients[fd].msg[j] == '\n') {
								clients[fd].msg[j] = '\0';
								sprintf(send_buffer, "client %d: %s\n", clients[fd].id, clients[fd].msg);
								ft_send(fd);
								bzero(clients[fd].msg, sizeof(clients[fd].msg));
								j = 1;
							}
						}
					}
				}
				break;
			}
		}
	}
	return(0);
}