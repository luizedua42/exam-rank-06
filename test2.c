#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>

typedef struct s_client {
	int id;
	char msg[300000];
}	t_client;

t_client clients[1024];
char sbuff[400000], rbuff[300000];
int maxfd = 0, gid = 0;
fd_set wfd, rfd, curr;

void err(char *msg) {
	if (msg)
		write(2, msg, strlen(msg));
	else
		write(2, "Fatal error", 11);
	write(2, "\n", 1);
	exit(1);
}

void broad(int e){
	for(int fd = 0; fd <= maxfd; fd ++)
		if(FD_ISSET(fd, &wfd) && fd != e)
			if(send(fd, sbuff, strlen(sbuff), 0) == -1)
				err(NULL);
}

int main (int ac, char **av) {
	if (ac != 2)
		err("Wrong number of arguments");

	struct sockaddr_in server;
	socklen_t len = 0;
	int serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if(serverfd == -1) err(NULL);
	maxfd = serverfd;

	FD_ZERO(&curr);
	FD_SET(serverfd, &curr);
	bzero(clients, sizeof(clients));
	bzero(&server, sizeof(server));

	server.sin_port = htons(atoi(av[1]));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(serverfd, (const struct sockaddr *)&server, sizeof(server)) == -1 || listen(serverfd, 100) == -1)
		err(NULL);

	while(1){
		wfd = rfd = curr;
		if(select(maxfd + 1, &rfd, &wfd, 0, 0) == -1) continue;
		
		for(int fd = 0; fd <= maxfd; fd++) {
			if(FD_ISSET(fd, &rfd)){
				if(fd == serverfd) {
					int clientfd = accept(fd, (struct sockaddr *)&server, &len);
					if(clientfd == -1) continue;
					if(clientfd > maxfd) maxfd = clientfd;
					clients[clientfd].id = gid++;
					FD_SET(clientfd, &curr);
					sprintf(sbuff, "server: client %d just arrived\n", clients[clientfd].id);
					broad(clientfd);
				} else {
					int ret = recv(fd, rbuff, sizeof(rbuff), 0);
					if(ret <= 0){
						sprintf(sbuff, "server: client %d just left\n", clients[fd].id);
						broad(fd);
						FD_CLR(fd, &curr);
						close(fd);
						bzero(clients[fd].msg, strlen(clients[fd].msg));
					} else {
						for(int i = 0, j = strlen(clients[fd].msg); i < ret; i++, j++) {
							clients[fd].msg[j] = rbuff[i];
							if(clients[fd].msg[j] == '\n'){
								clients[fd].msg[j] = '\0';
								sprintf(sbuff, "client %d: %s\n", clients[fd].id, clients[fd].msg);
								broad(fd);
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