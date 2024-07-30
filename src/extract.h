char *extract_user_agent(char *header,char *buf);
char *extract_host(char *headers);
char *extract_header(int incoming_sockfd);
char *extract_echo_string(char *path,char *buf);
char *extract_file_name(char *path,char *buf);
bool end_of_header(char *buf);
