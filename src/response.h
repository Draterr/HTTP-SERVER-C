#include <stdint.h>

typedef struct{
	int status;
	uint64_t content_length;
	char content_type[129];
	char *content_body;
}resp_t;

extern char *construct_response(char *buf, resp_t response_content);
extern bool is_malformed_request(char *path, char *method);
