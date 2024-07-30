#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>

char *extract_user_agent(char *header,char *buf);
char *extract_echo_string(char *path,char *buf);
char *extract_file_name(char *path,char *buf);
char *extract_host(char *headers);
bool end_of_header(char *buf);


bool end_of_header(char *buf){
	char *pmatch;
	pmatch = strstr(buf,"\r\n\r\n");
	if(pmatch != NULL){
		return true;
	}
	return false;
}

char *extract_file_name(char *path, char *buf){
	char file_name[255];
	if(strncmp(path, "/files/", 7) == 0){
		strtok(path, "/");
		snprintf(file_name, 255,"%s" , strtok(NULL, "\0"));
		strncpy(buf,file_name , strlen(file_name));
		return buf;
	}
	else{
		return NULL;
	}
}

char *extract_user_agent(char *header,char *buf){
	char *pUser_agent;
	pUser_agent = strstr(header, "User-Agent");
	if(pUser_agent != NULL){
		strtok(pUser_agent, " ");
		snprintf(buf,1024 , "%s", strtok(NULL, "\r"));
		return buf;
	}
	return NULL;
}


char *extract_echo_string(char *path,char *buf){
	char *pEcho;
	char *echo;
	if(strncmp(path, "/echo/", 6) == 0){
		strtok(path, "/");
		echo = strtok(NULL, "/");
		memcpy(buf, echo, strlen(echo));
		return buf;
	}
	return NULL;
}


char *extract_host(char *headers){
	char *host;
	char *current;
	char tmp[strlen(headers) + 1];
	memcpy(tmp, headers, strlen(headers)+1);
	host = strtok(tmp, "\r\n");
	while(host != NULL){
		host = strtok(NULL, "\r\n");
		current = strstr(host, "Host:"); 
		if(current != NULL){
			return current;
		}
	}
	return NULL;
}
