// Structs
typedef struct open_file_data {
	FILE * fp;
	int isActive;
} Open_File_Data;

// Functions
void intHandler(int signum);
int get_server_ip(char * ip_str);