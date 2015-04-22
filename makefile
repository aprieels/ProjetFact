all: fact

fact: fact.c
	gcc -o fact fact.c -lpthread

