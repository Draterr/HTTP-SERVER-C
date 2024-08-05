#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "response.h"
#include "extract.h"

char *construct_response(char *buf,resp_t response_content){
	time_t time_struct = time(NULL);
	char *current_time;
	if(time_struct){
		current_time = asctime(gmtime(&time_struct));
		current_time[strlen(current_time) - 1] = 0;
	}
	ulong snprintf_len = 0;
	if(response_content.status == 200){
	snprintf_len += strlen("HTTP/1.0 200 OK\r\n");
	}
	if(response_content.status == 404){
		snprintf_len += strlen("HTTP/1.0 404 Not Found\r\n");
	}
	char content_length_str[8];
	snprintf(content_length_str, 8, "%lu",response_content.content_length);

	snprintf_len += strlen("Content-Type: \r\n") + strlen(response_content.content_type);
	snprintf_len += strlen("Content-Length: \r\n") + strlen(content_length_str);
	snprintf_len += strlen("Date: \r\n") + strlen(current_time);
	snprintf_len += strlen("Server: nginY\r\n");
	snprintf_len += strlen("\r\n");

	if(response_content.content_body != NULL){snprintf_len += strlen(response_content.content_body);}

	//snprintf needs n+1 size to include null terminator
	if(response_content.status == 200){
		snprintf(buf, snprintf_len + 1,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %lu\r\nDate: %s\r\nServer: nginY\r\n\r\n%s",response_content.content_type,response_content.content_length,current_time,response_content.content_body);
	}
	if(response_content.status == 404){
		snprintf(buf, snprintf_len + 1,"HTTP/1.0 404 OK\r\nContent-Type: %s\r\nContent-Length: %lu\r\nDate: %s\r\nServer: nginY\r\n\r\n%s",response_content.content_type,response_content.content_length,current_time,response_content.content_body);
	}
	return buf;
}

bool is_malformed_request(char *path,char *method){
	char *tmp = malloc(4096);
	if(extract_file_name(path, tmp) == NULL){
		free(tmp);
		return true;
	}
	else if(!valid_method(method)){
		free(tmp);
		return true;
	}
	free(tmp);
	return false;

}
