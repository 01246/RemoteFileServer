all: netfileserver.c netfileserver.h netclient.c netclient.h libnetfiles.c libnetfiles.h
	make client
	make server

client: netclient.c netclient.h libnetfiles.c libnetfiles.h
	gcc -Wall -g -o client netclient.c libnetfiles.c

server: netfileserver.c netfileserver.h libnetfiles.c libnetfiles.h
	gcc -Wall -g -o server netfileserver.c libnetfiles.c -pthread

clean:
	rm -f client
	rm -f server
	rm -rf server.dSYM
	rm -rf client.dSYM
