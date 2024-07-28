#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include "extract.h"

//*******************************************************************************************************************
//*                                    FUNCTION DEFINITION							    *
//*******************************************************************************************************************
char *construct_response(int status,char *buf,int con_len,char *con_type,char *con);
char *get_arguments(int argc, char **argv);

#define min(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a < _b ? _a : _b; \
    })

//global variables
bool main_break = false;

int main(int argc, char** argv){
	// if(argc < 3){
	// 	printf("Usage: ./clone IP PORT\n");
	// 	exit(1);
	// }
	//
	char directory[4096];
	snprintf(directory, 4096, "%s", get_arguments(argc, argv));
	printf("%s\n",directory);
	printf("%lu\n",strlen(directory));
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct addrinfo *p;
	struct sockaddr_storage incoming;
	struct sockaddr_in *ipv4;
	socklen_t their_size = sizeof(incoming);
	char hbuf[NI_MAXHOST];
	char hserv[NI_MAXSERV];
	char *header;
	char *response;
	char method[8];
	int cnt;
	pid_t childpid;
	bool end_header;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = AI_PASSIVE;
	
	//populate addrinfo "ip,port,protocol,etc"

	// int getinfo = getaddrinfo(argv[1],argv[2],&hints,&servinfo);
	int getinfo = getaddrinfo("127.0.0.1","4221",&hints,&servinfo);
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
		printf("Clients Connected: %d\n",++cnt);

		//fork() returns twice 
		//1. the child process pid
		//2. returns 0 if its code that the child process should be running
		childpid = fork();
		if(childpid == -1){
			perror("fork");
			exit(-1);
		}
		if(childpid == 0){
			header = extract_header(accept_client);
			if(main_break){
				free(header);
				free(response);
				printf("Connection Closed from %s %s\n",hbuf,hserv);
				continue;
			}
			char request[2048];
			char *pstrtok;
			char *ptok;
			char *path;
			char version[10];
			char tmp_header[strlen(header)];
			if(strlen(header) != 0){
				memset(tmp_header, 0, strlen(header));
				memcpy(tmp_header, header, strlen(header));
				printf("tmp header: %s\n",tmp_header);
				path = malloc(2048);
				memset(path, 0, 2048);
				pstrtok = strtok(tmp_header, "\r\n");
				memcpy(request, pstrtok, 2048);
				ptok = strtok(request, " ");
				strncpy(method, ptok,min(8,strlen(ptok)));
				ptok = strtok(NULL, " ");
				strncpy(path, ptok,2048);
				ptok = strtok(NULL, "\r\n");
				strncpy(version, ptok,min(10,strlen(ptok)));
			}

			char cwd[4096];
			DIR *d;
			struct dirent *dir;
			if(strlen(directory) <= 0){
				if(getcwd(cwd, sizeof(cwd)) == NULL){
					perror("getcwd");
				}
			}
			else{
				d = opendir(directory);
				if(d == NULL){
					printf("Can't open directory\n");
					exit(-1);
				}
				else{
					strncpy(cwd, directory, 4096);
				}
			}
			printf("%s\n",request);
			printf("%s\n",path);
			printf("%s\n",version);
			char reply[1024];
			char user_agent[1024];
			char file_name[1024];
			char echo_string[1024];
			memset(reply,0, 1024);
			memset(echo_string,0, 1024);
			memset(user_agent,0, 1024);
			bool echo = false;
			strcat(cwd, "/");
			if(extract_file_name(path,file_name) != NULL){
				strncat(cwd,file_name,strlen(file_name));
				if(access(cwd, F_OK) == 0){
						printf("asdasdasdasd\n");
						construct_response(200, response,0,"text/plain",NULL);
						int send_client = send(accept_client,response,strlen(response),0);
						if(send_client == -1){
							perror("send");
							exit(-1);
						}
				}
				else{
					printf("tihissisi\n");
					construct_response(404, response,0,"text/plain",NULL);
					printf("%s\n",response);
					int send_client = send(accept_client,response,strlen(response),0);
					if(send_client == -1){
						perror("send");
						exit(-1);
					}
				}
			}
			if(extract_echo_string(path, echo_string) != NULL){
				// printf("%lu\n",strlen(echo_string));
				echo = true;
				if(strlen(reply) != 0){
					strncat(reply, echo_string,strlen(echo_string));
				}
				else{
					strncpy(reply, echo_string,strlen(echo_string));
				}
			}
			if(extract_user_agent(header, user_agent) != NULL){
				echo = true;	
				if(strlen(reply) != 0){
					strncat(reply, user_agent,strlen(user_agent));
				}
				else{
					strncpy(reply, user_agent,strlen(user_agent));
				}
			}
			response = malloc(2048);
			if(echo){
				construct_response(200, response,strlen(reply),"text/plain",reply);
				printf("response: %s\n",response);
				int send_client = send(accept_client,response,strlen(response),0);
				if(send_client == -1){
					perror("send");
					exit(-1);
				}
			}
			free(path);
			free(response);
			free(header);
			close(accept_client);
			}
	}
	close(sockfd);
	return 0;
}



char *construct_response(int status,char *buf,int con_len,char *con_type,char *con){
	printf("ddddd\n");
	if(con == NULL && status == 200){
		strcpy(buf, "HTTP/1.1 200 OK\r\n\r\n");
		printf("lllll\n");
		return buf;
	}
	else if(con == NULL && status == 404) {
		strcpy(buf,"HTTP/1.1 404 Not Found\r\n\r\n");
		printf("wtf\n");
		return buf;
	}
	printf("idk\n");
	ulong snprintf_len = 0;
	snprintf_len += strlen("HTTP/1.1 200 OK\r\n");
	snprintf_len += strlen("Content-Type: \r\n") + strlen(con_type);
	snprintf_len += strlen("Content-Length: \r\n") + sizeof(int);
	snprintf_len += strlen("\r\n");

	if(con != NULL){snprintf_len += strlen(con);}

	//snprintf needs n+1 size to include null terminator
	snprintf(buf, snprintf_len,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n%s",con_type,con_len,con);
	if(status == 200){
		return buf;
	}
	return buf;
}

char *extract_header(int incoming_sockfd){
		uint current = 0;
		uint h_size = 2048;
		char *tmp;
		char *header;
		char *new_ptr;
		header = malloc(2048);
		memset(header, 0, 2048);
		tmp = malloc(512);
		memset(tmp, 0, 512);
		while(end_of_header(tmp) == false){
			memset(tmp, 0, 512);
			int recv_client = recv(incoming_sockfd,tmp,512,0);
			if(recv_client == -1){
				perror("receive");
				exit(-1);
			}
			if(recv_client == 0){
				main_break = true;
				break;
			}
			current += strlen(tmp);
			if(current >= h_size){
				h_size = h_size * 2;
				new_ptr = realloc(header, h_size);
				if(new_ptr != NULL){
					header = new_ptr;
				}
				else{
					perror("realloc");
					exit(-1);
				}
			}
			strncat(header, tmp,strlen(tmp));
			printf("h_size = %u memory used = %u\n",h_size, current);
		}
		free(tmp);
		printf("end of header\n");
		return header;
}

char *get_arguments(int argc,char **argv){
	bool exist = false;
	int option_index = 0;
	static struct option long_options[] = {
		{"directory", required_argument,NULL,true}
	};
	exist = getopt_long(argc,argv ,"",long_options, &option_index);
	if(exist){
		return optarg;
	}
	return NULL;
}
