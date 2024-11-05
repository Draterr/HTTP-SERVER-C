#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/socket.h>
#include <time.h>
#include "extract.h"

char *extract_file_name(char *path,char *buf);
char *extract_host(char *headers);
bool end_of_header(char *buf);
bool valid_method(char *method);
encoding_t extract_encoding(char *headers);
void free_encoding(encoding_t encode);
char *extract_extension(char *request_file);


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

encoding_t extract_encoding(char *headers){
	encoding_t encoding = {.encoding_schemes = NULL , .available_schemes = 0};
	char *current;
	int i;
	char tmp[strlen(headers)+1];
	memcpy(tmp, headers, strlen(headers)+1);
	current = strstr(tmp,"Accept-Encoding: ");
	if(current != NULL){
		i = 0;
		encoding.encoding_schemes = malloc(sizeof(char *) * 6);
		int val;
		for(i=0;i<6;i++){
			encoding.encoding_schemes[i] = malloc(sizeof(char) *20);
		}
		i = 0;
		strtok(current, ":");
		current = strtok(NULL, ",");
		while(current != NULL){
			if(current[0] == ' '){
				current++;
			}
			strncpy(encoding.encoding_schemes[i], current,19);
			current = strtok(NULL, ",");
			encoding.available_schemes ++;
			i++;
		}
		encoding.encoding_schemes = realloc(encoding.encoding_schemes,sizeof(char *) * i);
		return encoding;
	}
	else{
		return encoding;
	}
}

char *extract_extension(char *request_file){
	char *tmp = strtok(request_file,".");
	char *extension;
	while(tmp != NULL){ //Iterate over all the dots in the file name to find the last extension
		extension = tmp; //set extension to the last one before tmp becomes a null pointer
		tmp = strtok(NULL,"."); //this is tmp because strtok will end up with a null pointer after we iterate over all the possible "." delimiters 
	}
	return extension;
}

void free_encoding(encoding_t encode){
	int i;
	i = 6;
	while(i < 6){
		free(encode.encoding_schemes[i]);
		i++;
	}
	free(encode.encoding_schemes);
}
