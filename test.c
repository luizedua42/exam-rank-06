#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>

typedef struct s_client {
	int id;
	char msg[300000];
}	t_clients;

t_clients clients[1024];

char s_buff[400000], r_buff[300000];
int maxfd = 0, gid = 0;
fd_set w_set, r_set, current;

void err(char *msg) {
	if(msg)
		write(2, msg, strlen(msg));
	else
		write(2, "Fatal error", 11);
	write(2, "\n", 1);
	exit(1);
}

void broadcast(int e) {
	for(int fd = 0; fd <= maxfd; fd++){
		if (FD_ISSET(fd, &w_set) && fd != e){
			if (send(fd, s_buff, strlen(s_buff), 0) == -1)
				err(NULL);
		}
	}
}

int main (int ac, char **av) {
	if (ac != 2)
		err("Wrong number of arguments");

	struct sockaddr_in server;
	socklen_t len;
	int serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if(serverfd == -1) err(NULL);
	maxfd = serverfd;

	FD_ZERO(&current);
	FD_SET(serverfd, &current);
	bzero(clients, sizeof(clients));
	bzero(&server, sizeof(server));

	server.sin_port = htons(atoi(av[1]));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(serverfd, (const struct sockaddr *)&server, len) == -1 || listen(serverfd, 100) == -1)
		err(NULL);

	while(1){
		w_set = r_set = current;
		if (select(maxfd+1, &r_set, &w_set, 0, 0) == -1) continue;

		for (int fd = 0; fd <= maxfd; fd++) {
			if(FD_ISSET(serverfd, &r_set)){
				if(fd == serverfd){
					int clientfd = accept(fd, (struct sockaddr *)&server, &len);
					if(clientfd == -1) continue;
					if(clientfd > maxfd) maxfd = clientfd;
					clients[clientfd].id = gid++;
					FD_SET(clientfd, &current);
					sprintf(s_buff, "server: client %d has just arrived\n", clients[clientfd].id);
					broadcast(clientfd);
				} else {
					int ret = recv(fd, r_buff, sizeof(r_buff), 0);
					if(ret <= 0) {
						sprintf(s_buff, "sever: client %d just left\n", clients[fd].id);
						broadcast(fd);
						FD_CLR(fd, &current);
						close(fd);
						bzero(clients[fd].msg,strlen(clients[fd].msg));
					} else {
						for (int i = 0, j = strlen(clients[fd].msg); i < ret; i++, j++){
							clients[fd].msg[j] = r_buff[i];
							if(clients[fd].msg[j] == '\n') {
								clients[fd].msg[j] = '\0';
								sprintf(s_buff, "client %d: %s", clients[fd].id, clients[fd].msg);
								broadcast(fd);
								bzero(clients[fd].msg, strlen(clients[fd].msg));
								j = -1;
							}
						}
					}
				}
				break;
			}
		}
	}
	return (0);
}