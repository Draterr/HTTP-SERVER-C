#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define CHUNK 16384 

char *read_from_file(FILE *fptr);
void remove_newline(char *word,int last_position);
int gzip_compress(const char *file_content, int input_size, char *output , int output_size);

int gzip_compress(const char *file_content, int input_size, char *output , int output_size){
	z_stream strm = {0};
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = (uInt)input_size;
	strm.next_in = (Bytef *)file_content;
	strm.avail_out = (uInt)output_size;
	strm.next_out = (Bytef *)output;
	deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,15|16, 8, Z_DEFAULT_STRATEGY);
	int def = deflate(&strm, Z_FINISH);
	if(def != Z_STREAM_END){
		perror("deflate");
		deflateEnd(&strm);
		return 0;
	}
	deflateEnd(&strm);
	return strm.total_out;
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
				if(buf == NULL){
					perror("realloc read_from_file");
				}
				curr_malloc_size = curr_malloc_size *2;
			}
		}
		else{
			perror("max memory reached\n");
			return buf;
		}
	}
	buf = realloc(buf, positon + 1);
	if(buf == NULL){
		perror("realloc read_from_file");
	}
	remove_newline(buf,positon);
	return buf;
}

void remove_newline(char *word,int last_position){
	int new_line_pos = last_position - 1;	
	if(word[new_line_pos] == '\n'){
		word[new_line_pos] = 0;
	}
}
