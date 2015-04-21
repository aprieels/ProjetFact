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

#define f(x)  (x*x+1)
#define BSIZE 100 //a faire varier

int getmaxthreads(int argc, char * argv[]);
void * consumer(void * arg);
int lecture(int argc, char * argv[]);
void addtobuffer(uint64_t nombre);
uint64_t readfrombuffer();

uint64_t buffer [BSIZE];
int index=0;
pthread_mutex_t buffermutex;	
sem_t empty;
sem_t full;

/*
 * Liste chainée de facteurs premiers
 *
 * @nbr : Facteur premier
 * @multpiple : 0 si le facteur n'apparait qu'une seule fois
 * @file : nom du premier fichier dans lequel apparait le facteur premier
 * @next : nombre premier suivant dans la liste chainée
 */
typedef struct primenumber {
	uint64_t nbr; 
	int multiple; 
	char[] file;
	PrimeNumber* next;
} PrimeNumber;

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
		pthread_create(&(threads[i]), NULL, &consumer, NULL); //consumer? argument?
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
	


/*
 *
 *
 *
 */
 void getnumbers(){
 	uint64_t number;
 	PrimeNumber* factorlist = malloc(sizeof(PrimeNumber*));
 	if(factorlist == NULL)
 		exit(EXIT_FAILURE);
 	while(1) {//Changer la condition par la suite pour que la boucle s'arrête lorsque tous les fichiers ont été lus !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 		number = readfrombuffer();
 		decomp(number, &factorlist);
 	}
 }


//Décomposition en facteurs premiers d'un nombre
void decomp(uint64_t nbr, PrimeNumber **list, char[] filename){
	
	int factor = pollard(nbr);
	if (factor == 0){
		factor = fact(nbr);
		if (factor == 0){
			addprimefactor(nbr, list, filename);
		}
		else {
			decomp(factor);
			decomp(nbr/factor);
		}
	}
	else {
		decomp(factor);
		decomp(nbr/factor);
	
}


	
/*
 * pollard
 * Cherche un facteur d'un nombre donné en utilisant la méthode Rho de Pollard
 * Attention : Cette méthode est plus rapide que la méthode naïve mais n'est pas infaillible
 *
 * @nbr : Nombre ddont il faut trouver un facteur
 * @return : Facteur de n trouvé, ou 0 si aucun facteur n'a été trouvé
 */
uint64_t pollard(uint64_t nbr){
	
	if(nbr==1)
		return 0;
	
	int i;	
	uint64_t x = rand() % (nbr+1); //Choisi un nombre aléatoirement entre 0 et n
	uint64_t y = x;
	int k = 2;
	uint64_t d = 0;
	
	for(i=0; d != nbr ; i++){ //La condition d!=nbr permet uniquement d'éviter un livelock si le nombre est premier. Si un facteur n'est pas trouvé assez vite, on utilisera la méthode naïve pour vérifier que ce nombre est bien premier
	
		x = f(x) % nbr;
		d = gcd(abs(y-x), nbr);
		
		if((d != 1) && (d != nbr)){
			return d;
		}
		
		if(i==k){
			y = x;
			k=2*k;
		}
		
	}
	
	return 0;

}


/*
 * gcd
 * Calcule le PGCD de deux nombres en utilisant l'algorithme d'Euclide
 *
 * @a et @b : Deux nombres dont il faut calculer le PGCD
 * @return : PGCD des deux nombres
 */
uint64_t gcd(uint64_t a, uint64_t b){

    uint64_t c;
    while (b != 0)
    {
        c = a % b;
        a = b;
        b = c;
    }
    return a;
    
}
 
/*
 * fact
 * Cherche un facteur premier d'un nombre donné en utilisant la méthode naïve
 *
 * @nbr : Nombre dont il faut trouver un facteur
 * @return : Premier facteur trouvé, ou 0 si aucun facteur n'a été trouvé (nbr est donc premier)
 */ 
uint64_t fact(uint64_t nbr){
	
	uint64_t i;
	
	if (nbr % 2 == 0) //Vérifie que 2 n'est pas un facteur
		return 2;
	
	for(i=3; i <= nbr; i=i+2){ //Ne teste pas les nombres pairs car, puisque 2 n'est pas un facteur, ces multiples ne peuvent pas non plus en être
	
		if (nbr % i == 0)
			return i; //Retourne i si c'est un diviseur de nbr
	
	} 
	
	return 0; //Aucun diviseur n'a été trouvé, nbr est donc premier
}
	
//Ajoute un facteur à la liste
void addprimefactor(uint64_t factor, PrimeNumber **factorlist, char[] filename){
	
	PrimeNumber *newprime;
			
	if(*factorlist == NULL){
	
		newprime = (PrimeNumber*) malloc(sizeof(PrimeNumber));
		if(newprime == NULL)
			exit(EXIT_FAILURE);
			
		newprime -> nbr = factor;
		newprime -> multiple = 0;
		newprime -> file = filename;
		newprime -> next = NULL;
		
		*factorlist == newprime;
		
	}
	else {
	
		PrimeNumber *currentprime = *factorlist;
		PrimeNumber *nextprime = current -> next;
		
		if(factor < (*currentprime).nbr){
			newprime = (PrimeNumber*) malloc(sizeof(PrimeNumber));
			if(newprime == NULL)
				exit(EXIT_FAILURE);
			newprime -> nbr = factor;
			newwprime -> multiple = 0;
			newprime -> file = filename;
			newprime -> next = *factorlist;
			*factorlist = newprime;
		}
		else {
			
			while((*current).nbr)
			
	}

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

