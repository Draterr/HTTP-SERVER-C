#include <stdint.h>
#include <assert.h>
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
#include <time.h>
#include "extract.h"

//*******************************************************************************************************************
//*                                    FUNCTION DEFINITION							    *
//*******************************************************************************************************************
char *construct_response(int status,char *buf,int con_len,char *con_type,char *con);
char *get_arguments(int argc, char **argv);
char *read_from_file(FILE *file);
void remove_newline(char *word, int last_position);
void free_mem_close_sock(char *path, char *header, char *response, char *reply, int socket);

#define min(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a < _b ? _a : _b; \
    })

//global variables
bool main_break = false;
char not_found_name[] = {"/response/404.html"};
char current_dir[4096] = {0};
int main(int argc, char** argv){
	// if(argc < 3){
	// 	printf("Usage: ./clone IP PORT\n");
	// 	exit(1);
	// }
	//
	char directory[4096];
	char *optarg;
	optarg = get_arguments(argc, argv);
	if(optarg){
		snprintf(directory, strlen(optarg)+1, "%s", optarg);
	}
	else{
		printf("Usage: ./server --directory=(directory_to_serve)\n");
		exit(-3);
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
	char *response;
	char method[8];
	int cnt = 0;
	pid_t childpid;
	bool end_header;

//*******************************************************************************************************************
//*                                         SETTING UP SOCKETS
//*******************************************************************************************************************
	memset(&hints, 0, sizeof(struct addrinfo));
	//AF_INET = IPV4
	hints.ai_family = AF_INET;
	//SOCK_STREAM = TCP(kinda)
	hints.ai_socktype = SOCK_STREAM;

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
		if(p->ai_family == AF_INET && p->ai_socktype == SOCK_STREAM){
			ipv4 = (struct sockaddr_in *)p->ai_addr;
			char ip[INET_ADDRSTRLEN];
			inet_ntop(p->ai_family,&(ipv4->sin_addr),ip,sizeof(ip));
			printf("IP: %s\n",ip);
			uint16_t port = ntohs(ipv4->sin_port);
			printf("PORT: %hu\n",port);
		}
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
//*******************************************************************************************************************
//*                                        HTTP Related Code 
//*******************************************************************************************************************
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
			char version[10] = {0};
			char tmp_request[strlen(header)+1];
			char *host;
			if(strlen(header) != 0){
				memcpy(tmp_request, header, strlen(header)+1);
				// printf("tmp header: %s\n",tmp_request);
				path = malloc(2048);
				memset(path, 0, 2048);
				pstrtok = strtok(tmp_request, "\r\n");
				memcpy(request, pstrtok, 2048);
				ptok = strtok(request, " ");
				strncpy(method, ptok,min(8,strlen(ptok)));
				ptok = strtok(NULL, " ");
				strncpy(path, ptok,2048);
				ptok = strtok(NULL, "\r\n");
				strncpy(version, ptok,min(10,strlen(ptok)));
				host = extract_host(header);
			}
			char cwd[4096] = {0};
			if(getcwd(current_dir, sizeof(current_dir)) == NULL){
				perror("getcwd");
			}
			DIR *d;
			struct dirent *dir;
			d = opendir(directory);
			if(d == NULL){
				printf("Can't open directory\n");
				exit(-1);
				closedir(d);
			}
			else{
				strncpy(cwd, directory, strlen(directory));
				closedir(d);
			}
			printf("%s\n",request);
			printf("%s\n",path);
			printf("%s\n",version);
			char *reply;
			char user_agent[1024] = {0};
			char file_name[1024] = {0};
			char echo_string[1024] = {0};
			bool echo = false;
			if(cwd[strlen(cwd)-1] != '/'){
				strcat(cwd, "/");
			}
			reply = malloc(1024);
			memset(reply, 0, 1024);
			if(strcmp(request, "GET") == 0){
				if(extract_file_name(path,file_name) != NULL){
					strncat(cwd,file_name,strlen(file_name));
					if(access(cwd, F_OK) == 0){
							FILE *fptr = fopen(cwd, "r");					
							if(fptr == NULL){
								perror("fopen");
								}
							response = read_from_file(fptr);
							reply = realloc(reply, strlen(response) + 2048);
							construct_response(200, reply,strlen(response),"application/octet-stream",response);
							int send_client = send(accept_client,reply,strlen(reply),0);
							if(send_client == -1){
								perror("send");
								exit(-1);
							}
						fclose(fptr);
						free_mem_close_sock(path, header, response, reply, accept_client);
						continue;
					}
					else{
						char not_found_path[4096] = {0};
						strncpy(not_found_path, current_dir, sizeof(current_dir));
						strncat(not_found_path, not_found_name, strlen(not_found_name));
						FILE *not_found = fopen(not_found_path, "r");
						if(!not_found){
							perror("fopen response file");
							printf("Failed to open the responses at %s\n",not_found_path);
						}
						response = read_from_file(not_found);
						construct_response(404, reply,strlen(response),"text/html",response);
						int send_client = send(accept_client,reply,strlen(reply),0);
						if(send_client == -1){
							perror("send");
							exit(-1);
						}
						free_mem_close_sock(path, header, response, reply, accept_client);
						fclose(not_found);
						continue;
					}
				}
			}
			else if(strcmp(request, "POST") == 0) {
				strncat(cwd,file_name,strlen(file_name));
			}
			free_mem_close_sock(path, header, response, reply, accept_client);
			continue;
			}
	}
	close(sockfd);
	return 0;
}



char *construct_response(int status,char *buf,int con_len,char *con_type,char *con){
	time_t time_struct = time(NULL);
	char *current_time;
	if(time_struct){
		current_time = asctime(gmtime(&time_struct));
		current_time[strlen(current_time) - 1] = 0;
	}
	ulong snprintf_len = 0;
	if(status == 200){
	snprintf_len += strlen("HTTP/1.0 200 OK\r\n");
	}
	if(status == 404){
		snprintf_len += strlen("HTTP/1.0 404 Not Found\r\n");
	}
	snprintf_len += strlen("Content-Type: \r\n") + strlen(con_type);
	snprintf_len += strlen("Content-Length: \r\n") + sizeof(int);
	snprintf_len += strlen("Date: \r\n") + strlen(current_time);
	snprintf_len += strlen("Server: nginY\r\n");

	if(con != NULL){snprintf_len += strlen(con);}
	//snprintf needs n+1 size to include null terminator
	if(status == 200){
		snprintf(buf, snprintf_len,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\nDate: %s\r\nServer: nginY\r\n\r\n%s",con_type,con_len,current_time,con);
	}
	if(status == 404){
		snprintf(buf, snprintf_len,"HTTP/1.0 404 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\nDate: %s\r\nServer: nginY\r\n\r\n%s",con_type,con_len,current_time,con);
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

char *read_from_file(FILE *fptr){
	char *buf;
	char tmp[2048];
	size_t positon;
	size_t curr_mem;
	size_t curr_malloc_size;
	uint32_t max_mem = UINT32_MAX;
	curr_malloc_size = 2048;
	curr_mem = 0;
	positon = 0;
	buf = malloc(2048);
	if(buf == NULL){
		perror("malloc");
		return NULL;
	}
	while(fgets(tmp, 2048, fptr)){
		memcpy(&buf[positon], tmp, strlen(tmp));
		positon += strlen(tmp);
		curr_mem += strlen(tmp)+1;
		if(curr_malloc_size < max_mem){
			if(curr_mem >= curr_malloc_size){
				buf = realloc(buf, curr_malloc_size * 2);
				curr_malloc_size = curr_malloc_size *2;
			}
		}
		else{
			perror("max memory reached\n");
			return buf;
		}
	}
	buf = realloc(buf, positon + 1);
	fseek(fptr, 0, SEEK_SET);
	remove_newline(buf,positon);
	printf("read_from_file: %s\n",buf);
	return buf;
}

void remove_newline(char *word,int last_position){
	int new_line_pos = last_position - 1;	
	if(word[new_line_pos] == '\n'){
		word[new_line_pos] = 0;
	}
}

void free_mem_close_sock(char *path, char *header, char *response, char *reply, int socket){
	free(path);
	free(header);
	free(reply);
	free(response);
	close(socket);
}
