#include <stdint.h>

typedef struct resp_t{
	int status;
	uint64_t content_length;
	char content_type[129];
	char *content_body;
}resp_t;

typedef union is_malformed_u {
	bool is_malformed;
	char file_name[1024];
}is_malformed_u;

typedef struct is_malformed_s{
	int uniontype; //0 = bool, 1 = file_name
	is_malformed_u inner_union;
}is_malformed_s;


extern char *construct_response(char *buf, resp_t response_content);
extern is_malformed_s is_malformed_request(char *path, char *method);