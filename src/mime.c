#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *get_mime_type(const char *file_extension);
char *open_mime_type_file();

char *open_mime_type_file()
{
	FILE *mime_type_file = fopen("/etc/mime.types","r");
	long size;
	char *file_content = NULL;
	char tmp_buffer[1024] = {0};
	uint mem_size = 1048576;
	uint mem_used = 0;
	uint MAX = 1677216;
	uint mem_left = mem_size;

	file_content = calloc(mem_size+1,sizeof(char));
	if(file_content == NULL){
		perror("malloc");
	}
	if(mime_type_file == NULL){
		perror("Error opening /etc/mime.types");
		exit(-1);
	}
	
	while(fgets(tmp_buffer,1024,mime_type_file) != NULL){
		size_t len_str = strlen(tmp_buffer);
		if(mem_left < 1024 && mem_used >= mem_size && mem_size < MAX){
			mem_size *= 2;	
			printf("Allocating more memory\n");
			file_content = realloc(file_content,mem_size);
			if(file_content == NULL){
				free(file_content);
				perror("realloc");
				exit(-1);
			}
			mem_left = mem_size - mem_used;
		}
		else if(mem_size >= MAX){
			perror("MAX amount of memory reached!");
			printf("Exiting...\n");
			exit(-1);
		}

		if(tmp_buffer[0] != '#')
		{
			if(mem_left < len_str){
				exit(-1);
			}
			strncat(file_content,tmp_buffer,mem_left);
			mem_used += len_str;
			mem_left = mem_size - mem_used;
		}
		memset(tmp_buffer,0,1024);
	}
	fclose(mime_type_file);
	return file_content;
}
//iterate over /etc/mime.types and check for extension
char *get_mime_type(const char *file_extension)
{
	char *mime_types = open_mime_type_file();
	char *content_type;
	char *ext;
	char *individual_ext;
	char *line_save_ptr;
	char *extension_save_ptr;
	char *individual_ext_save_ptr;
	char *result;

	content_type = strtok_r(mime_types, "\n", &line_save_ptr);
	while(content_type != NULL){
		ext = strtok_r(content_type, "				", &extension_save_ptr);
		ext = strtok_r(NULL, "				", &extension_save_ptr);
		if(ext != NULL){
			individual_ext = strtok_r(ext," ",&individual_ext_save_ptr);
			while(individual_ext != NULL)
			{
				if(strcmp(file_extension,individual_ext) == 0){
					printf("content_type:%s\n",content_type);
					printf("ext:%s\n",individual_ext);
					result = strndup(content_type,strlen(content_type));
					if(result == NULL){
						perror("result");
					}
					break;
				}
				individual_ext = strtok_r(NULL," ",&individual_ext_save_ptr);
			}
		}
		content_type = strtok_r(NULL, "\n", &line_save_ptr);
	}
	free(mime_types);
	return result;
}

