CFLAGS = -c -Wall
CC = gcc
LIBS =  -lm 

all: client server

client: client.c checkinput.c
	${CC} client.c checkinput.c -o client -pthread

server: server.c serverHelper.c checkinput.c
	${CC} server.c serverHelper.c checkinput.c -o server -pthread

clean:
	rm -f *.o *~
