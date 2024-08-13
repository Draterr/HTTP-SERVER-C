#include <stdio.h>
#include <stdint.h>

extern char *read_from_file(FILE *fptr);
extern void remove_newline(char *word,int last_position);
extern int gzip_compress(char *file_content, unsigned int input_size, char *output , unsigned int output_size);
