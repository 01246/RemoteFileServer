// Structs
typedef struct open_file_data {
	FILE * fp;
	int isActive;
} Open_File_Data;

// Functions
int readn(int fd, char * ptr, int nbytes);
int writen(int fd, char * ptr, int nbytes);
int doSomething(int sockfd);