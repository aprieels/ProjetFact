#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <endian.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include "netread.h"

#define f(x)  (x*x+1)
#define BSIZE 1000 //a faire varier

/*
 * structure contenant un nombre et le fichier duquel il provient
 *
 * @nombre : nombre à factoriser
 * @nomfichier : nom du fichier d'origine
 */
typedef struct{
	uint64_t nombre;
	char * nomfichier; // transformer en short correspondant à un nom de fichier?
} numberandname;

/*
 * Liste chainée de facteurs premiers
 *
 * @nbr : Facteur premier
 * @multiple : 0 si le facteur n'apparait qu'une seule fois
 * @file : nom du premier fichier dans lequel apparait le facteur premier
 * @next : nombre premier suivant dans la liste chainée
 */
typedef struct primenumber{
	uint32_t nbr; 
	int multiple; 
	struct primenumber* next;
	char file[];
} PrimeNumber;


int getmaxthreads(int argc, char * argv[]);
int filescount (int argc, char * argv[]);
int lecture(int argc, char * argv[]);
void addtobuffer(numberandname * nan);
numberandname * readfrombuffer();
PrimeNumber * merge(int nthr, PrimeNumber * retvals[]);
PrimeNumber * findsolution(PrimeNumber* finallist);

numberandname * buffer[BSIZE];
int index=-1; //index du prochain nombre du buffer a factoriser
pthread_mutex_t buffermutex;	
sem_t empty;
sem_t full;

int finishedreading=0;
//"booléen", qnd lecteur a fini de lire tt les fichiers, finishedreading==1



int main(int argc, char * argv[]){
	struct timeval tv1;
	struct timeval tv2;
	gettimeofday(&tv1, NULL);

	//récupérer le nombre max de threads passé en argument
	int nthr=getmaxthreads(argc, argv);
	if(nthr==0)
		return EXIT_FAILURE;
	

	//initialisation des mutex/sémaphores associées au buffer à partager entre producers et consumers
	pthread_mutex_init( &buffermutex, NULL);	
	sem_init(&empty, 0, BSIZE);
	sem_init(&full, 0, 0);

	//déterminer le nombre de fichiers à lire pr savoir combien de thread producteur il va falloir lancer
	int prodnumber=filescount (argc, argv);

	//lancer la lecture des fichiers pour alimenter le buffer
	pthread_t threadsprod[prodnumber];
	int a;
	for(a=0; a<prodnumber; a++){



		pthread_create(&(threadsprod[a]), NULL, &lecture, NULL); 


	}

	//lancer les threads consumer pour factoriser les nombres
	pthread_t threadscons [nthr];
	int i;
	for(i=0; i<nthr; i++){
		pthread_create(&(threadscons[i]), NULL, NULL, NULL); //consumer? argument?
	}

	//on récuppère les threads producteurs
	int b;
	for(b=0; b<nthr; b++){
		pthread_join(threadsprod[b], NULL); // NULL pr la valeur de retour?
	}

/*
	//lancer la lecture des fichiers pour alimenter le buffer
	int e=lecture(argc, argv);
	if(e!=0)
		return EXIT_FAILURE;//fermer threads d'abord?

*/


	finishedreading=1;//la lecture des fichiers est terminée
	sem_post(&full);//pour débloquer les threads consumers (similaire à problème du rdv)


	//récupère les listes chainées de chaque thread
	PrimeNumber * retvals [nthr]; 
	int j;
	for(j=0; j<nthr; j++){
		pthread_join(threadscons[i], (void **)&(retvals[i]));
	}


	//merge les listes chainées
	PrimeNumber * finallist = merge(nthr, retvals);	
	

	//trouve le résultat, et printf
	PrimeNumber * solution = findsolution(finallist);
	if(solution->nbr==0){
		perror("no solution found");
		return EXIT_FAILURE;
	}
	gettimeofday(&tv2, NULL);
	printf("%i\n", solution->nbr);
	printf("%s\n", solution->file);
	printf("%i.%i\n", tv2->tv_sec - tv1->tv_sec, tv2->tv_usec - tv1->tv_usec);
}
	


/*
 *
 *
 *
 */
 void getnumbers(){
 	numberandname nan;
 	PrimeNumber* factorlist = malloc(sizeof(PrimeNumber*));
 	if(factorlist == NULL)
 		exit(EXIT_FAILURE);
 	nan = readfrombuffer();
 	while(nan.nombre!=0) {
 		decomp(nan.nombre, &factorlist, nan.nomfichier);
 		nan = readfrombuffer();
 	}
 }


//Décomposition en facteurs premiers d'un nombre
void decomp(uint64_t nbr, PrimeNumber **list, char filename[]){
	
	uint64_t factor = pollard(nbr);
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
	
/*
 * addprimefactor
 * Ajoute un facteur premier à la liste chainée triée contenant les facteurs
 * Si ce facteur ce trouve déja dans la liste, il change la variable "multiple" de la structure contenant le facteur
 *
 * @factor64 : Facteur premier à ajouter à la liste sous la forme uint64_t
 * @factorlist : Pointeur vers les pointeur du premier élément de la liste
 * @filename : fichier duquel le facteur premier est tiré
 */
void addprimefactor(uint64_t factor64, PrimeNumber **factorlist, char filename[]){
	
	PrimeNumber *newprime;
	uint32_t factor = factor64 & 0xFFFFFFFF; //Transforme le facteur, qui est initialement stocké sur 64 bits en format uint32_t
			
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
		else{
			while((*nextprime).nbr < factor && nextprime->next != NULL){
				currentprime = nextprime;
				nextprime = nextprime -> next;
			}
			if((*nextprime).nbr == factor){
				(*nextprime).multiple = 1;
			}
			else if((*nextprime).nbr > factor) {
				newprime -> nbr = factor;
				newprime -> multiple = 0;
				newprime -> file = filename;
				newprime -> next = nextprime;
				currentprime -> next = newprime;
			}
			else {
				newprime -> nbr = factor;
				newprime -> multiple = 0;
				newprime -> file = filename;
				newprime -> next = NULL;
				nextprime -> next = newprime;
			}
		}
	}

}
	
	

/*
 * getmaxthread
 * a partir de la ligne de commande, renvoie le nombre max de threads utilisables donnee
 *
 * argc : argc de main
 * argv : argv de main
 */
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

/*
 * filescount
 * a partir de la ligne de commande, renvoie le nombre de fichiers à lire (en incluant -stdio comme un fichier)
 *
 * argc : argc de main
 * argv : argv de main
 */
int filescount (int argc, char * argv[]){
	int files=0;
	for(i=1; i<argc; i++){
		if(strcmp(argv[i], "-stdin")==0 || strcmp(argv[i],"file")==0 || strncmp(argv[i],"http://",7)==0)
			files++;
	}
	return files;
}


/*
 * lecture
 * lis l'ensemble des fichiers, c'est à dire transforme tous les nombres de tous les fichiers en représentation locale
 * et les dispose dans buffer
 *
 * argc : argc de main
 * argv : argv de main
 */
int lecture(int argc, char * argv[]){
	int isstdin=0; //vaudra 1 s'il faut lire les arguments passés en standard in
	int descr;
	int e;
	uint64_t nombre;
	numberandname * nan;
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
				nan=(numberandname *)malloc(sizeof(numberandname));
				if(nan==NULL){
					return EXIT_FAILURE; // compléter
				}
				nan->nombre=be64toh(nombre);//nombre transformé en représentation locale
				nan->nomfichier=argv[i+1];
				addtobuffer(nan);
			}
			//fermer le fichier
			if(close(descr)!=0){
				perror("close");
				return EXIT_FAILURE;
			}
		}
		else if (strncmp(argv[i],"http://",7)==0){
			//ouvrir le fichier depuis le reseau
			const char * mode="r";
			URL_FILE *url =url_fopen(argv[i],mode);
			if(url==NULL){
				perror("openurl");
				return EXIT_FAILURE;
			}
			//lire le fichier depuis le reseau
			while(url_feof(url)==0){//tant que le fichier n'est pas terminé
				if((int)url_fread(&nombre, sizeof(uint64_t), 1, url)==0){
					perror("readurl");
					return EXIT_FAILURE;
				}
				nan=(numberandname *)malloc(sizeof(numberandname));
				if(nan==NULL){
					return EXIT_FAILURE; // compléter
				}
				nan->nombre=be64toh(nombre);
				nan->nomfichier=argv[i+1];
				addtobuffer(nan);
			}
			//fermer le fichier
			if(url_fclose(url)!=0){
				perror("closeurl");
				return EXIT_FAILURE;
			}
		}
	}
	if(isstdin==1){//s'il faut lire des nombres de la ligne de commande
		while( (e=read(0, &nombre, sizeof(uint64_t))) != 0){
			if(e<0){
				perror("readstdin");
				return EXIT_FAILURE;
			}
			nan=(numberandname *)malloc(sizeof(numberandname));
			if(nan==NULL){
				return EXIT_FAILURE; // compléter
			}
			nan->nombre=be64toh(nombre);//nombre transformé en représentation locale
			nan->nomfichier=argv[i+1];
			addtobuffer(nan);
		}
	}
}

/*
 * addtobuffer
 * rajoute le numberandname passé en argument à buffer en respectant le protocole des producers-consumers
 *
 * nan : structure numberandname a ajouter au buffer
 *
 */
void addtobuffer(numberandname * nan){
	sem_wait(&empty);
	pthread_mutex_lock(&buffermutex);
		index++;
		buffer[index]=nan;
	pthread_mutex_unlock(&buffermutex);
	sem_post(&full);	
}

/*
 * readfrombuffer
 * retire et renvoie un numberandname de buffer en respectant le protocole des producers-consumers
 * renvoie un pointeur vers un numberandname avec un nombre=0 si le buffer est vide est la lecture de tous les fichiers est terminée
 * 
 */
numberandname * readfrombuffer(){
	numberandname * nan;
	sem_wait(&full);
	pthread_mutex_lock(&buffermutex);
		if(index==-1 && finishedreading==1){ // pas tt à fait sur
			sem_post(&full);
			nan=(numberandname *)malloc(sizeof(numberandname));
			if(nan==NULL){
				return NULL; // compléter
			}
			nan->nombre=0;
			nan->nomfichier="x";
		}
		else{
			nan=buffer[index];
			index--;
		}
	pthread_mutex_unlock(&buffermutex);
	sem_post(&empty);
	return nan;
}

/*
 * merge
 * merge les listes chainées ordonnées de chaque thread en une seule liste chainée ordonnée
 * renvoie un pointeur vers le premier élement de la liste chainée finale
 * 
 * retvals[] : liste des premiers élements des listes chainées associées à chaque thread
 * nthr : nombre de threads utilisés pour la factorisation, taille de retvals
 */
PrimeNumber * merge(int nthr, PrimeNumber * retvals[]){
	PrimeNumber * finallist=(PrimeNumber *)malloc(sizeof(PrimeNumber));
	if(finallist==NULL){
		return NULL;
	}
	finallist->nbr=0;
	finallist->multiple=0;
	finallist->file="x";
	finallist->next=retvals[0];
	// on prend la liste chainée du premier thread, puis on y rajoute succésivement les listes chainées des autres threads.
	PrimeNumber * currentnode;
	PrimeNumber * nextnode;
	PrimeNumber * addnode;
	PrimeNumber * nextaddnode;
	int i;
	for(i=1; i<nthr; i++){ 
		currentnode=finallist;
		nextnode=currentnode->next;
		addnode=retvals[i];
		while(addnode != NULL){
			if(nextnode==NULL){
				currentnode->next=addnode;
				addnode=NULL;
			}
			else{
				if(addnode->nbr == nextnode->nbr){//si le nombre a insérer est déjà dans la liste principale
					nextnode->multiple=1;
					addnode=addnode->next;
				}
				else if(addnode->nbr < nextnode->nbr){//s'il faut insérer addnode entre currentnode et nextnode
					currentnode->next=addnode;
					nextaddnode=addnode->next;
					addnode->next=nextnode;
					nextnode=addnode;
					addnode=nextaddnode;
				}
				currentnode=nextnode;
				nextnode=nextnode->next;
			}
		}			
	}
	return finallist;
}

/*
 * findsolution
 * trouve le plus grand facteur premier unique présent dans une liste chainée ordonnée dans l'ordre croissant
 * 
 * finallist : liste chainée finale contenant tous les facteurs premiers trouvé dans les fichiers
 */
PrimeNumber * findsolution(PrimeNumber* finallist){
	PrimeNumber * solution=finallist;//parce qu'on sait que le premier élément de finalist est 0
	while(finallist!=NULL){
		if(finallist->multiple==0){
			solution=finallist;
		}
		finallist=finallist->next;
	}
	return solution;
}
