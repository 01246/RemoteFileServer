// Structs
typedef struct open_file_data {
	FILE * fp;
	int isActive;
} Open_File_Data;

// Functions
int get_server_ip(char * ip_str);
int executeClientCommands(int * sockfd);