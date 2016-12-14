// Structs
typedef struct open_file_data {
	FILE * fp;
	int isActive;
} Open_File_Data;

typedef struct thread_data {
	int * cli_fd;
	int client_id;

} Thread_data;

// Functions
int get_server_ip(char * ip_str);
int executeClientCommands(Thread_data * td);