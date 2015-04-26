all: fact.c

fact.c: netread.o
	gcc fact.c netread.c -lcurl -lpthread -o fact




