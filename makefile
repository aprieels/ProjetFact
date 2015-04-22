all: fact

fact: fact.c
	gcc -o fact fact.c -lpthread netread.c $(pkg-config --libs --cflags libcurl)

