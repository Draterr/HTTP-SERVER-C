#include <stdio.h>
#include <stdint.h>

typedef struct file_content_properties{
	uint64_t file_size;
	char *file_contents;
}content_prop;

extern content_prop *read_from_file(FILE *fptr);
extern void remove_newline(char *word,int last_position);
extern int gzip_compress(const char *file_content, int input_size, char *output , int output_size);
