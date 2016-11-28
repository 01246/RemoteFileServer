//Server
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "netfileserver.h"
#include "libnetfiles.h"

#define SERV_TCP_PORT 8001

int readn(int fd, char * ptr, int nbytes){
	int nleft, nread;

	nleft = nbytes;
	while(nleft > 0){
		nread = read(fd,ptr,nleft);
		if(nread < 0){
			//error
			return(nread);
		}else if(nread == 0){
			//EOF
			break;
		}
		nleft -= nread;
		ptr +=nread;
	}
	return(nbytes-nleft);
}

int writen(int fd, char * ptr, int nbytes){
	int nleft, nwritten;

	nleft = nbytes;
	while(nleft > 0){
		nwritten = write(fd,ptr,nleft);
		if(nwritten <= 0){
			// ERROR
			return(nwritten);
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (nbytes-nleft);
}

int doSomething(int sockfd){
	//Sits on new socket to handle client library requests
	for(;;){
		printf("In do something for loop\n");
		char * buffer[256];
		Command_packet packet;
	//read command packet from socket 
		readn(sockfd,(char*)&packet,sizeof(packet));
		printf("Command_packet type: %d", packet.type);

			//switch on Command_packet type variable

			//read additional data if necessary 

			//execute command

			//populate return command packet with status 

			//write command packet to socket

			//write additional data if necessary 


	}
}

int main(int argc, char *argv[]){

	/*
		socket file descriptors
	*/
	int sockfd, newsockfd; socklen_t clilen;

	/*
		Structures, sockaddr_in is defined in library sys/sockets.h
	*/
	struct sockaddr_in serv_addr, cli_addr;

	if (argc < 2){
   		printf("ERROR, no port provided\n");
  		 exit(1);
 	}


	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("server: can't open steam socket\n");
	}


	/*
		zero out struct serv_addr
		Initialize domain (ser_addr.sin_family)
		Initialize server socket internet address, prepares to bind socket to all available interfaces 
		Initialize server socket to wait on certain specified port
	*/

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERV_TCP_PORT);

	/*
		Binds server socket to a specific port, bind must happen in order for listen to happen
	*/

	if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		printf("Can't bind local address\n");
	}
	/*
		Server waits for incoming connection requestions
		sockfd will be the socket to satisfy these requests 
	*/
	listen(sockfd,5);
	pthread_t clientThreads[100];
	int flag = 0;
	int i = 0;

	//sits on accept, waiting for new clients
	for(; ;){
		printf("In main loop top\n");
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		printf("Received accept: blocking \n");

		if(newsockfd < 0){
			printf("server: accept error");
		}
		if((flag = pthread_create(&clientThreads[i], NULL, (void *)doSomething, (void *)&sockfd))){
			printf("Threading error");
		}else if(flag == 0){
			printf("CONNECTION MADE\n");
			i++; 
			
		}

		 
		


	}
	

}






