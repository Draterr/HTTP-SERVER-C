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
#include "response.h"


//*******************************************************************************************************************
//*                                    FUNCTION DEFINITION							    *
//*******************************************************************************************************************
char *get_arguments(int argc, char **argv);
char *read_from_file(FILE *file);
void remove_newline(char *word, int last_position);
char *add_base_path(char *file_name);
void free_mem_close_sock(char *path, char *header,char *reply, int socket);

#define min(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a < _b ? _a : _b; \
    })

//global variables
bool main_break = false;
char not_found_name[] = {"/response/404.html"};
char bad_request_name[] = {"/response/400.html"};
char response_file_path[4096] = {0};
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
				strncpy(method, ptok,sizeof(method));
				ptok = strtok(NULL, " ");
				strncpy(path, ptok,2048);
				ptok = strtok(NULL, "\r\n");
				strncpy(version, ptok,sizeof(method));
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
				strncpy(cwd, directory, sizeof(cwd));
				closedir(d);
			}
			if(cwd[strlen(cwd)-1] != '/'){
				strcat(cwd, "/");
			}

			printf("Whole Request: %s\n",request);
			printf("Request_Path: %s\n",path);
			printf("HTTP_Version: %s\n",version);
			char *reply;
			reply = malloc(1024);
			memset(reply, 0, 1024);
			char user_agent[1024] = {0};
			char file_name[1024] = {0};
			char echo_string[1024] = {0};
			bool echo = false;
			resp_t four_o_four = {.status = 404, .content_length = 0, .content_type = {0}, .content_body = NULL};
			resp_t two_hundred = {.status = 200, .content_length = 0, .content_type = {0}, .content_body = NULL};
			resp_t four_hundred = {.status = 400, .content_length = 0, .content_type = {0}, .content_body = NULL};
			is_malformed_s malformed; 

			malformed = is_malformed_request(path,method);
			if(malformed.uniontype == 0){
				add_base_path(bad_request_name);
				FILE *fptr = fopen(response_file_path, "r");
				if(!fptr){
					perror("fopen 400");
					continue;
				}
				four_hundred.content_body = read_from_file(fptr);
				four_hundred.content_length = strlen(four_hundred.content_body);
				strncpy(four_hundred.content_type, "text/html", 129);
				reply = realloc(reply, strlen(four_hundred.content_body) + 2048);
				construct_response(reply, four_hundred);
				int send_client = send(accept_client,reply,strlen(reply),0);
				if(send_client == -1){
					perror("send");
					exit(-1);
				}
				free(four_hundred.content_body);
				fclose(fptr);
				free_mem_close_sock(path, header, reply, accept_client);
				continue;
			}else if(malformed.uniontype == 1){
				strncpy(file_name,malformed.inner_union.file_name,sizeof(file_name));
				strncat(cwd,file_name,strlen(file_name));
			}
			if(strcmp(request, "GET") == 0){
					if(access(cwd, F_OK) == 0){
							FILE *fptr = fopen(cwd, "r");					
							if(fptr == NULL){
								perror("fopen");
								}
							two_hundred.content_body = read_from_file(fptr);
							two_hundred.content_length = strlen(two_hundred.content_body);
							strncpy(four_hundred.content_type, "application/octet-stream", 129);
							reply = realloc(reply, strlen(two_hundred.content_body) + 2048);
							construct_response(reply, two_hundred);
							printf("read_from_file: %s\n",reply);
							int send_client = send(accept_client,reply,strlen(reply),0);
							if(send_client == -1){
								perror("send");
								exit(-1);
							}
						fclose(fptr);
						free(two_hundred.content_body);
						free_mem_close_sock(path, header, reply, accept_client);
						continue;
					}
					else{
						add_base_path(not_found_name);
						FILE *not_found = fopen(response_file_path, "r");
						if(!not_found){
							perror("fopen response file");
							printf("Failed to open the response at %s\n",response_file_path);
						}
						four_o_four.content_body = read_from_file(not_found);
						four_o_four.content_length = strlen(four_o_four.content_body);
						strncpy(four_o_four.content_type, "text/html", 129);
						reply = realloc(reply, strlen(four_o_four.content_body) + 2048);
						construct_response(reply, four_o_four);
						int send_client = send(accept_client,reply,strlen(reply),0);
						if(send_client == -1){
							perror("send");
							exit(-1);
						}
						free_mem_close_sock(path, header, reply, accept_client);
						free(four_o_four.content_body);
						fclose(not_found);
						continue;
					}
				}
			else if(strcmp(request, "POST") == 0) {
				strncat(cwd,file_name,strlen(file_name));
			}
			free_mem_close_sock(path, header, reply, accept_client);
			continue;
			}
	}
	close(sockfd);
	return 0;
}


char *extract_header(int incoming_sockfd){
		ulong current = 0;
		uint h_size = 2048;
		char *tmp;
		char *header;
		char *new_ptr;
		int i = 0;
		header = malloc(2048);
		memset(header, 0, 2048);
		tmp = malloc(513);
		memset(tmp, 0, 513);
		while(!end_of_header(tmp)){
			memset(tmp, 0, 513);
			int recv_client = recv(incoming_sockfd,tmp,512,0);
			if(recv_client == -1){
				perror("receive");
				exit(-1);
			}
			if(recv_client == 0){
				main_break = true;
				break;
			}
			if(recv_client > 0){
				tmp[512] = 0;
				current += strlen(tmp) + 1;
			}
			printf("h_size = %u, memory used = %lu\n",h_size, current);
			if(current >= h_size){
				h_size = h_size * 2;
				if(h_size > UINT16_MAX){
					printf("Maximum amount of memory exceeded!\n");
					printf("Aborting...\n");
					main_break = true;
					break;
				}
				new_ptr = realloc(header, h_size);
				if(new_ptr != NULL){
					printf("reallocing...\n");
					header = new_ptr;
				}
				else{
					perror("realloc");
					exit(-1);
				}
			}
			strncat(header, tmp,strlen(tmp));
		}
		free(tmp);
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
	return buf;
}

void remove_newline(char *word,int last_position){
	int new_line_pos = last_position - 1;	
	if(word[new_line_pos] == '\n'){
		word[new_line_pos] = 0;
	}
}

char *add_base_path(char *file_name){
	strncpy(response_file_path, current_dir, sizeof(response_file_path));
	strncat(response_file_path, file_name, strlen(file_name));
	return response_file_path;
}

void free_mem_close_sock(char *path, char *header, char *reply, int socket){
	free(path);
	free(header);
	free(reply);
	close(socket);
}
