#include <stdint.h>

typedef struct resp_t{
	int status;
	uint64_t content_length;
	char content_type[129];
	char *content_body;
	int encoding; //0 - no encoding, 5 - gzip, 4 - br, 3 - deflate, 2 - compress , 1 - zstd
}resp_t;

typedef union is_malformed_u {
	bool is_malformed;
	char file_name[1024];
}is_malformed_u;

typedef struct is_malformed_s{
	int uniontype; //0 = bool, 1 = file_name
	is_malformed_u inner_union;
}is_malformed_s;

typedef struct response_info{
	char *buf;
	unsigned int header_len;
	unsigned int buf_size;
}resp_info;

typedef struct resposne_types{
	int type;
	char *content_type;
} responese_type;


extern resp_info construct_response(resp_info response_information, resp_t response_content,int data_type,unsigned int buffer_size);
extern is_malformed_s is_malformed_request(char *path, char *method);
