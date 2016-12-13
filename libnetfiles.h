// Structs
typedef struct command_packet {
	int type;		// type of command (open, close, read, write)
	int flag;		// ???
	int size;		// number of bytes in read or write
	int status;		// file descriptor
} Command_packet;

// Functions
void * get_in_addr(struct sockaddr *sa);
int netserverinit(char * hostname);
void writeCommand(int sockfd, int type, int flag, int size, int status);
void *  readCommand(int sockfd);
int readn(int fd, char * ptr, int nbytes);
int writen(int fd, char * ptr, int nbytes);
int netopen(const char *pathname, int flags);
int netclose(int fd);
ssize_t netread(int fildes, void *buf, size_t nbyte);
ssize_t netwrite(int fildes, const void *buf, size_t nbyte);