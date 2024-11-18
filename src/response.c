#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include "response.h"
#include "extract.h"


resp_info construct_response(resp_info response_information,resp_t response_content,int data_type,uint buffer_size){//data_type = 1 means compressed data, 0 means regular data
	printf("%s\n",response_content.content_type);
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
	if(response_content.status == 400){
		snprintf_len += strlen("HTTP/1.0 400 Bad Request\r\n");
	}
	char content_length_str[8];
	snprintf(content_length_str, 8, "%lu",response_content.content_length);
	snprintf_len += strlen("Content-Type: \r\n") + strlen(response_content.content_type);
	snprintf_len += strlen("Content-Length: \r\n") + strlen(content_length_str);
	snprintf_len += strlen("Date: \r\n") + strlen(current_time);
	snprintf_len += strlen("Server: nginY\r\n");
	snprintf_len += strlen("\r\n");
	
	if(data_type == 1){snprintf_len += strlen("Content-Encoding: gzip\r\n");}
	response_information.header_len = snprintf_len;
	if(response_content.content_body != NULL && data_type == 0){snprintf_len += response_content.content_length;}
	if(response_information.header_len + response_content.content_length > buffer_size){
		buffer_size = response_content.content_length + response_information.header_len;
		response_information.buf = realloc(response_information.buf,buffer_size);
		if(response_information.buf == NULL){
			perror("construct_response realloc");
		}
	}
	response_information.buf_size = buffer_size;
	//snprintf needs n+1 size to include null terminator
	if(data_type == 0){
		if(response_content.status == 200 && data_type == 0){
			snprintf(response_information.buf, snprintf_len + 1,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %lu\r\nDate: %s\r\nServer: nginY\r\n\r\n%s",response_content.content_type,response_content.content_length,current_time,response_content.content_body);
		}
		if(response_content.status == 404 && data_type == 0){
			snprintf(response_information.buf, snprintf_len + 1,"HTTP/1.0 404 Not Found\r\nContent-Type: %s\r\nContent-Length: %lu\r\nDate: %s\r\nServer: nginY\r\n\r\n%s",response_content.content_type,response_content.content_length,current_time,response_content.content_body);
		}
		if(response_content.status == 400 && data_type == 0){
			snprintf(response_information.buf, snprintf_len + 1,"HTTP/1.0 400 Bad Request\r\nContent-Type: %s\r\nContent-Length: %lu\r\nDate: %s\r\nServer: nginY\r\n\r\n%s",response_content.content_type,response_content.content_length,current_time,response_content.content_body);
		}

	}
	else{
		if(response_content.status == 200 && data_type == 1){
			snprintf(response_information.buf, snprintf_len + 1,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %lu\r\nDate: %s\r\nServer: nginY\r\nContent-Encoding: gzip\r\n\r\n",response_content.content_type,response_content.content_length,current_time);
			printf("%lu\n",response_content.content_length);
			memcpy(&response_information.buf[snprintf_len], response_content.content_body, response_content.content_length);
		}
		if(response_content.status == 404 && data_type == 1){
			snprintf(response_information.buf, snprintf_len + 1,"HTTP/1.0 404 Not Found\r\nContent-Type: %s\r\nContent-Length: %lu\r\nDate: %s\r\nServer: nginY\r\nContent-Encoding: gzip\r\n\r\n",response_content.content_type,response_content.content_length,current_time);
			memcpy(&response_information.buf[snprintf_len], response_content.content_body, response_content.content_length);
		}
		if(response_content.status == 400 && data_type == 1){
			snprintf(response_information.buf, snprintf_len + 1,"HTTP/1.0 400 Bad Request\r\nContent-Type: %s\r\nContent-Length: %lu\r\nDate: %s\r\nServer: nginY\r\nContent-Encoding: gzip\r\n\r\n",response_content.content_type,response_content.content_length,current_time);
			memcpy(&response_information.buf[snprintf_len], response_content.content_body, response_content.content_length);
		}
	}
	return response_information;
}

is_malformed_s is_malformed_request(char *path,char *method){
	is_malformed_s return_union = {.uniontype = 0, .inner_union.file_name = {0}};
	char tmp[1024];
	if(!access(path,R_OK)){
		return_union.uniontype = 0;
		return_union.inner_union.is_malformed = true;
		return return_union;
	}
	if(extract_file_name(path, tmp) == NULL){
		return_union.uniontype = 0;
		return_union.inner_union.is_malformed = true;
		return return_union;
	}
	else if(!valid_method(method)){
		return_union.uniontype = 0;
		return_union.inner_union.is_malformed = true;
		return return_union;
	}
	
	return_union.uniontype = 1;
	memcpy(return_union.inner_union.file_name , tmp, 1024);
	return return_union;
}
