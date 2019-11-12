CFLAGS = -g -Wall -lpthread
CC = gcc

# change based on type of router to be built
# value can be either DISTVECTOR or PATHVECTOR
ROUTERMODE = PATHVECTOR

# if DEBUG is 1, debugging messages are printed
DEBUG = 0

# Check which OS
OS := $(shell uname)
ifeq ($(OS), SunOS)
	SOCKETLIB = -lsocket
endif

all : router

endian.o   :   ne.h endian.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -c endian.c

routingtable.o   :   ne.h routingtable.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -c routingtable.c

router  :   endian.o routingtable.o router.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -D DEBUG=$(DEBUG) endian.o routingtable.o router.c -o router -lnsl $(SOCKETLIB)

unit-test  : routingtable.o unit-test.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -D DEBUG=$(DEBUG) routingtable.o unit-test.c -o unit-test -lnsl $(SOCKETLIB)

clean :
	rm -f *.o
	rm -f router
	rm -f unit-test

runRouter0 :
	./router 0 localhost 2097 3097

runRouter1 :
	./router 1 localhost 2097 4097

runRouter2 :
	./router 2 localhost 2097 5097

runRouter3 :
	./router 3 localhost 2097 6097
