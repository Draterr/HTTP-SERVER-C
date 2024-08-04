#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/socket.h>

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
	char *tmp;
	int i;
	int j = 0;
	int directory_depth = 0;
	
	//getting the depth of the path
	for(i=0;i < strlen(path);i++){
		if(path[i] == '/'){
			directory_depth++;	
		}
	}
	tmp = strtok(path, "/");
	
	//find the filename
	while(j < directory_depth - 1){
		j++;
		tmp = strtok(NULL,"/");
	}
	snprintf(file_name, 255,"%s", tmp);
	strncpy(buf,file_name , strlen(file_name));
	return buf;
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
