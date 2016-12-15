server: netfileserver.c netfileserver.h libnetfiles.c libnetfiles.h
	gcc -Wall -g -o server netfileserver.c libnetfiles.c -pthread

clean:
	rm -f server
	rm -rf server.dSYM
