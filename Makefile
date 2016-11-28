all: netfileserver.c netfileserver.h netclient.c netclient.h
	make libnetfiles
	make client
	make server

client: netclient.c netclient.h libnetfiles.c libnetfiles.h
	gcc -Wall -g -o client netclient.c libnetfiles.o

server: netfileserver.c netfileserver.h
	gcc -Wall -g -o server netfileserver.c

libnetfiles: libnetfiles.c libnetfiles.h
	gcc -Wall -g -c libnetfiles.c 	

clean:
	rm -f libnetfiles.o
	rm -f client
	rm -f server
	rm -rf server.dSYM
	rm -rf client.dSYM
