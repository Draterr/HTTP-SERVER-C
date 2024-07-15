#include <asm-generic/socket.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

char *construct_response(int status,char *buf){
	if(status == 200){
		strcpy(buf,"HTTP/1.0 200 OK\r\n");
		return buf;
	}
	return buf;
}

int parse_method(char *method){
	
}

bool end_of_header(char *buf){
	char *pmatch;
	pmatch = strstr(buf,"\r\n\r\n");
	if(pmatch != NULL){
		return true;
	}
	return false;
}
int main(int argc, char** argv){
	if(argc < 3){
		printf("Usage: ./clone IP PORT\n");
		exit(1);
	}
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct addrinfo *p;
	struct sockaddr_storage incoming;
	struct sockaddr_in *ipv4;
	socklen_t their_size = sizeof(incoming);
	char hbuf[NI_MAXHOST];
	char hserv[NI_MAXSERV];
	char *header;
	char *tmp;
	char *response;
	uint current;
	uint header_size = 2048;
	char method[8];
	bool end_header;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = AI_PASSIVE;
	
	//populate addrinfo "ip,port,protocol,etc"

	int getinfo = getaddrinfo(argv[1],argv[2],&hints,&servinfo);
	if(getinfo != 0){
		fprintf(stderr,"getaddrinfo error: %s\n", gai_strerror(getinfo));
		exit(-1);
	}
	//create a network socket with the addrinfo struct

	int sockfd = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if(sockfd == -1){
		perror("socket");
		exit(-1);
	}
	//iterate over addrinfo linked list and print out the port and ip

	for(p = servinfo; p != NULL; p = p->ai_next){
		ipv4 = (struct sockaddr_in *)p->ai_addr;
		char ip[INET_ADDRSTRLEN];
		inet_ntop(p->ai_family,&(ipv4->sin_addr),ip,sizeof(ip));
		printf("IP: %s\n",ip);
		uint16_t port = ntohs(ipv4->sin_port);
		printf("PORT: %d\n",port);
	}
	//setsockopt allows us to reuse the same ip and port preventing the PORT ALREADY IN USE ERRORS
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1){
		perror("setsockopt");
		exit(-1);
	}

	//bind the network socket to a port
	int bind_soc = bind(sockfd,servinfo->ai_addr,servinfo->ai_addrlen);
	if(bind_soc == -1){
		perror("bind");
		exit(-1);
	}

	//get the network socket to actually listen with 3 max queue
	int listen_socket = listen(sockfd, 3);
	if(listen_socket == -1){
		perror("listen");
		exit(-1);
	}

	freeaddrinfo(servinfo);
	while(1){
		//accept any incoming connections on the network socket (BLOCKING)
		int accept_client = accept(sockfd, (struct sockaddr *)&incoming, &their_size);

		if(accept_client == -1){
			perror("accept");
			exit(-1);
		}

		if(getnameinfo((struct sockaddr *)&incoming, their_size, hbuf, sizeof(hbuf), hserv, sizeof(hserv),NI_NUMERICHOST |NI_NUMERICSERV) == -1){
			perror("getnameinfo");
		};
		
		printf("Connection received from %s %s\n",hbuf,hserv);
		header_size = 2048;
		header = malloc(header_size);
		response = malloc(1024);
		tmp = malloc(512);
		memset(header, 0, header_size);
		memset(tmp, 0, 512);
		while(end_of_header(tmp) == false){
			memset(tmp, 0, 512);
			int recv_client = recv(accept_client,tmp,512,0);
			if(recv_client == -1){
				perror("receive");
				exit(-1);
			}
			if(recv_client == 0){
				break;
			}
			current = strlen(header);
			if(current >= header_size){
				header_size = header_size *2;
				header = realloc(header, header_size);
			}
			strcat(header, tmp);
			printf("%s\n",header);
			printf("%u\n",header_size);
		}
		printf("end of header\n");
		// memcpy(method, header, 4);
		// parse_method(method);
		// construct_response(200, response);
		// int send_response = send(accept_client,reply,1024,0);
		// if(send_response == -1){
		// 	perror("send");
		// 	exit(-1);
		// }
		// memset(header, 0, 1024);
		// free(response);
		free(tmp);
		free(response);
		free(header);
		close(accept_client);
	}
	close(sockfd);
	return 0;
}
