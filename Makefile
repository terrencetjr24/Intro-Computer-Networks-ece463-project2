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

endian.o   :   header/ne.h source/endian.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -c source/endian.c

routingtable.o   :   header/ne.h source/routingtable.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -c source/routingtable.c

router  :   endian.o routingtable.o source/router.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -D DEBUG=$(DEBUG) endian.o routingtable.o router.c -o router -lnsl $(SOCKETLIB)

unit-test  : routingtable.o source/unit-test.c
	$(CC) $(CFLAGS) -D $(ROUTERMODE) -D DEBUG=$(DEBUG) routingtable.o unit-test.c -o unit-test -lnsl $(SOCKETLIB)

clean :
	rm -f *.o
	rm -f router
	rm -f unit-test
