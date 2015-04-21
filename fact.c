#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl\curl.h>
#include <endian.h>

int getmaxthreads(int argc, char * argv[]);
void * consumer(void * arg);
int lecture(int argc, char * argv[]);

int main(int argc, char * argv[]){
	//récupérer le nombre max de threads passé en argument
	int nthr=getmaxthreads(argc, argv);
printf("maxthr=%i\n", nthr);
	if(nthr==0)
		return EXIT_FAILURE;



	//lance les threads consumer pour lire les nombres
	pthread_t threads [nthr];
	int i;
	for(i=0; i<nthr; i++){
		pthread_create(&(threads[i]), NULL, &consumer, ***); //consumer?
	}


	//lancer la lecture des fichiers pour alimenter le buffer
	int e=lecture(argc, argv);
	if(e!=0)
		return EXIT_FAILURE;
	
	//la lecture des fichiers est terminée
	//récupère les listes chainées de chaque thread
	void * retval [nthr]; //malloc?bof, void?
	int j;
	for(j=0; j<nthr; j++){
		pthread_join(threads[i], &(retval[i]));
	}


	//merge les listes chainées
	


	//trouve le résultat, et printf
}

int getmaxthreads(int argc, char * argv[]){
	int nthr=0;
	int a;
	for(a=1; a<argc-1; a++){
		if(strcmp(argv[a], "-maxthreads")==0){
				nthr=atoi(argv[a+1]);//ajouter sécurité?
		}
	}
	return nthr;
}

void * consumer(void * arg){

}

int lecture(int argc, char * argv[]){
	int isstdin=0; // vaudra 1 s'il faut lire les arguments passés en standard in
	int i;
	for(i=1; i<argc; i++){
		if(strcmp(argv[i], "-stdin")==0)
			isstdin=1;
		else if (strcmp(argv[i],"file")==0){
			//ouvrir fichier de nom argv[i+1]
		}
		else if (strncmp(argv[i],"http://",7)==0){
			//lire le fichier depuis le reseau
		}
	}
	if(isstdin==1){ //s'il faut lire des nombres de la ligne de commande

	}
}
