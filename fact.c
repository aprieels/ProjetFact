#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl\curl.h>
#include <endian.h>



int main(int argc, char * argv[]){
	//récupérer le nombre max de threads
	int nthr=0;
	int a;
	for(a=0; a<argc, a++){
		switch(argv[a]
	nthr=***
	//lance le thread producer pour lire les fichiers


	//lance les threads consumer pour lire les nombres
	pthread_t threads [nthr];
	int i;
	for(i=0; i<nthr; i++){
		pthread_create(&(threads[i]), NULL, &consumer, ***); //consumer?
	}
	
	//récupère les listes chainées de chaque thread
	void * retval [nthr]; //malloc?bof, void?
	int j;
	for(j=0; j<nthr; j++){
		pthread_join(threads[i], &(retval[i]));
	}

	//merge les listes chainées
	***

	//trouve le résultat, et printf
}
