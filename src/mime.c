#include <stdio.h>
#include <magic.h>

void get_mime_type(const char *file_name);

void get_mime_type(const char *file_name){
	const char* mime;
	magic_t magic;

	magic = magic_open(MAGIC_MIME_TYPE);
	magic_load(magic,NULL);
	magic_compile(magic, NULL);
	mime = magic_file(magic, file_name);

	printf("%s\n",mime);
	magic_close(magic);

}

