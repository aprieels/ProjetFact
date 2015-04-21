#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl\curl.h>
#include <endian.h>


//structure qui peut contenir les arguments de la fonction main, permet de les transmettre a un thread
typedef struct{
	int argc;
	char * argv[];//argv[argc]?
} main_args;
	


void * consumer(void * arg){

}

void * producer(void * arg){
	main_args * args=(main_args *) arg;
	
}



int main(int argc, char * argv[]){
	//récupérer le nombre max de threads passé en argument
	int nthr=0;
	int a;
	for(a=0; a<argc-1; a++){
		if(strcmp(argv[a], "-maxthreads")==0){
				nthr=atoi(argv[a+1]);//ajouter sécurité?
		}
	}
printf("maxthr=%i\n", nthr);
	if(nthr==0)
		return EXIT_FAILURE;


	//lance le thread producer pour lire les fichiers
	main_args args={argc, argv};
	pthread_t lecteur;
	pthread_create(&lecteur, NULL, &producer, (void *)&args);


	//lance les threads consumer pour lire les nombres
	pthread_t threads [nthr];
	int i;
	for(i=0; i<nthr; i++){
		pthread_create(&(threads[i]), NULL, &consumer, ***); //consumer?
	}


	//rapelle le thread lecteur
	pthread_join(lecteur, NULL);
	

	//récupère les listes chainées de chaque thread
	void * retval [nthr]; //malloc?bof, void?
	int j;
	for(j=0; j<nthr; j++){
		pthread_join(threads[i], &(retval[i]));
	}


	//merge les listes chainées
	


	//trouve le résultat, et printf
}
