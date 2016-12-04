// Structs
typedef struct command_packet {
	int type;
	int flag;
	int size;
	int status;
} Command_packet;

// Functions
int netserverinit(char * hostname);
int netopen(const char *pathname, int flags);
int netclose(int fd);
ssize_t netread(int fildes, void *buf, size_t nbyte);
ssize_t netwrite(int fildes, const void *buf, size_t nbyte);
int readn(int fd, char * ptr, int nbytes);
int writen(int fd, char * ptr, int nbytes);
void writeCommand(int sockfd, int type, int flag, int size, int status);
Command_packet *  readCommand(int sockfd);
