default: server

server: clean
	gcc -g -Wall -Wextra -Werror -pthread webserver.c -o webserver

dist: clean
	tar -cvzf 105125631.tar.gz webserver.c Makefile README

clean:	
	rm -rf webserver *.o *.tar.gz