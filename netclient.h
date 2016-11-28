typedef struct socket{
	int sockfd;
	int newsockfd;
	int clilen;
	int childpid;
	int cli_addr;
	int serv_addr;
} Socket;
