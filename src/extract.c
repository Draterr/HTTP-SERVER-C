#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/socket.h>
#include <time.h>

char *extract_file_name(char *path,char *buf);
char *extract_host(char *headers);
bool end_of_header(char *buf);
bool valid_method(char *method);


bool end_of_header(char *buf){
	char *pmatch;
	pmatch = strstr(buf,"\r\n\r\n");
	if(pmatch != NULL){
		return true;
	}
	return false;
}

bool valid_method(char *method){
	char *valid_methods[] = {"GET","POST","HEAD","OPTIONS","PUT","DELETE","TRACE","PATCH"};
	int v = 0;
	while(v < sizeof(valid_methods) / sizeof(char *)){
		if(strncmp(method,valid_methods[v],strlen(method)) == 0){
			return true;		
		}
		v++;
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
	strncpy(buf,file_name , 1024);
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

char **extract_encoding(char *headers){
	char **encoding = malloc(sizeof(char *) * 6);
	char *current;
	char tmp[strlen(headers)+1];
	int i;
	int val;
	for(i=0;i<6;i++){
		encoding[i] = malloc(sizeof(char) *20);
	}
	memcpy(tmp, headers, strlen(headers)+1);
	current = strstr(tmp,"Accept-Encoding: ");
	i = 0;
	if(current != NULL){
		strtok(current, ":");
		current = strtok(NULL, ",");
		while(current != NULL){
			if(current[0] == ' '){
				current++;
			}
			strncpy(encoding[i], current,19);
			current = strtok(NULL, ",");
			i++;
		}
		encoding = realloc(encoding,sizeof(char *) * i);
		return encoding;
	}
	else{
		return NULL;
	}
}

void free_encoding(char **encoding){
	int i;
	i = 0;
	while(encoding[i] != NULL){
		free(encoding[i]);
		i++;
	}
	free(encoding);
}
