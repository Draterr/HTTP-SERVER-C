extern char *extract_user_agent(char *header,char *buf);
extern char **extract_encoding(char *headers);
extern char *extract_host(char *headers);
extern void free_encoding(char **encoding);
extern char *extract_header(int incoming_sockfd);
extern char *extract_echo_string(char *path,char *buf);
extern char *extract_file_name(char *path,char *buf);
extern bool end_of_header(char *buf);
extern bool valid_method(char *method);

