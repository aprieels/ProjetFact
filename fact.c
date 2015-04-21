#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
//#include <curl/curl.h>
#include <endian.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int getmaxthreads(int argc, char * argv[]);
void * consumer(void * arg);
int lecture(int argc, char * argv[]);
void addtobuffer(uint64_t nombre);
uint64_t readfrombuffer();

#define BSIZE 100 //a faire varier
uint64_t buffer [BSIZE];
int index=0;
pthread_mutex_t buffermutex;	
sem_t empty;
sem_t full;



int main(int argc, char * argv[]){
	//récupérer le nombre max de threads passé en argument
	int nthr=getmaxthreads(argc, argv);
printf("maxthr=%i\n", nthr);
	if(nthr==0)
		return EXIT_FAILURE;
	
	//initialisation du buffer à partager entre producers et consumers
	pthread_mutex_init( &buffermutex, NULL);	
	sem_init(&empty, 0, BSIZE);
	sem_init(&full, 0, 0);


	//lance les threads consumer pour factoriser les nombres
	pthread_t threads [nthr];
	int i;
	for(i=0; i<nthr; i++){
		pthread_create(&(threads[i]), NULL, &consumer, NULL); //consumer?
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
	int isstdin=0; //vaudra 1 s'il faut lire les arguments passés en standard in
	int descr;
	int e;
	uint64_t nombre;
	int i;
	for(i=1; i<argc; i++){
		if(strcmp(argv[i], "-stdin")==0)
			isstdin=1;
		else if (strcmp(argv[i],"file")==0){
			//ouvrir le fichier
			if((descr=open(argv[i+1], O_RDONLY))<0){
				perror("open");
				return EXIT_FAILURE;
			}
			//lire le fichier
			while( (e=read(descr, &nombre, sizeof(uint64_t))) != 0){
				if(e<0){
					perror("read");
					return EXIT_FAILURE;
				}
				nombre=be64toh(nombre);//nombre transformé en représentation locale
				addtobuffer(nombre);

			}
			//fermer le fichier
			if(close(descr)!=0){
				perror("close");
				return EXIT_FAILURE;
			}
		}
		else if (strncmp(argv[i],"http://",7)==0){
			//lire le fichier depuis le reseau
		}
	}
	if(isstdin==1){//s'il faut lire des nombres de la ligne de commande

	}
}

void addtobuffer(uint64_t nombre){
	sem_wait(&empty);
	pthread_mutex_lock(&buffermutex);
		buffer[index]=nombre;
		index++;
	pthread_mutex_unlock(&buffermutex);
	sem_post(&full);	
}

uint64_t readfrombuffer(){
	uint64_t nombre;
	sem_wait(&full);
	pthread_mutex_lock(&buffermutex);
		nombre=buffer[index];
		index--;
	pthread_mutex_unlock(&buffermutex);
	sem_post(&empty);
	return nombre;
}

