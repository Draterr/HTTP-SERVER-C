#include <stdio.h>
#include <stdint.h>

extern char *read_from_file(FILE *fptr);
extern void remove_newline(char *word,int last_position);
extern int gzip_compress(const char *file_content, int input_size, char *output , int output_size);
