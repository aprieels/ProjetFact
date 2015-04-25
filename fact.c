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
//#include "netread.h"

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
	struct primenumber * next;
	char * file;
} PrimeNumber;

uint64_t pollard(uint64_t);
void addprimefactor(uint64_t, PrimeNumber*, char*);
void * getnumbers(void * arg);
void decomp(uint64_t, PrimeNumber*, char*);
uint64_t pollard(uint64_t);
uint64_t fact(uint64_t);
uint64_t gcd(uint64_t, uint64_t);

int getmaxthreads(int argc, char * argv[]);
int filescount (int argc, char * argv[]);
void * readfile(void * filename);
void * readURL(void * urlname);
void * readstdin(void * arg);
void addtobuffer(numberandname * nan);
numberandname * readfrombuffer();
PrimeNumber * merge(int nthr, PrimeNumber * retvals[]);
PrimeNumber * findsolution(PrimeNumber * finallist);
void freelinkedlist(PrimeNumber * finallist);

numberandname * buffer[BSIZE];
int ind=-1; //index du prochain nombre du buffer a factoriser
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
	int consnumber=getmaxthreads(argc, argv);
printf("consnumber=%i\n", consnumber);
	if(consnumber==0)
		return EXIT_FAILURE;
	
	//initialisation des mutex/sémaphores associées au buffer à partager entre producers et consumers
	pthread_mutex_init( &buffermutex, NULL);	
	sem_init(&empty, 0, BSIZE);
	sem_init(&full, 0, 0);

	//déterminer le nombre de fichiers à lire pr savoir combien de threads producteur il va falloir lancer
	int prodnumber=filescount (argc, argv);
printf("prodnumber=%i\n", prodnumber);

	//lancer la lecture des fichiers pour alimenter le buffer
	pthread_t threadsprod[prodnumber];
	int a=0;
	int i;
	for(i=0; i<argc; i++){
		if(strcmp(argv[i], "-stdin")==0){
			pthread_create(&(threadsprod[a]), NULL, &readstdin, NULL);
			a++;
		}			
		else if (strcmp(argv[i],"file")==0){
			pthread_create(&(threadsprod[a]), NULL, &readfile, (void *)argv[i+1]); 
			a++;
		}
		else if (strncmp(argv[i],"http://",7)==0){
			pthread_create(&(threadsprod[a]), NULL, &readURL, (void *)argv[i]);
			a++; 
		}
	}

	//lancer les threads consumer pour factoriser les nombres
	pthread_t threadscons [consnumber];
	for(i=0; i<consnumber; i++){
		pthread_create(&(threadscons[i]), NULL, &getnumbers, NULL);
	}

	//on récuppère les threads producteurs
	for(i=0; i<prodnumber; i++){
		pthread_join(threadsprod[i], NULL); // NULL pr la valeur de retour?
	}

	finishedreading=1;//la lecture des fichiers est terminée
	sem_post(&full);//pour débloquer les threads consumers (similaire à problème du rdv)

	//récupère les listes chainées de chaque thread
	PrimeNumber * retvals [consnumber]; 
	for(i=0; i<consnumber; i++){
		pthread_join(threadscons[i], (void **)&(retvals[i]));
	}
printf("%i\n", (int)((retvals[1])->next)->nbr);
	//merge les listes chainées
	PrimeNumber * finallist = merge(consnumber, retvals);	

	//trouve le résultat, et printf
	PrimeNumber * solution = findsolution(finallist);
	if(solution->nbr==0){
		perror("no solution found");
		return EXIT_FAILURE;
	}

	//temps écoulé
	gettimeofday(&tv2, NULL);
	printf("%i\n", solution->nbr);
	printf("%s\n", solution->file);
	printf("%i.%i\n", (int)(tv2.tv_sec - tv1.tv_sec), (int)(tv2.tv_usec - tv1.tv_usec));//à corriger
	
	//free finallist
	freelinkedlist(finallist);
}
	


/*
 *
 *
 *
 */
void * getnumbers(void * arg){
 	numberandname* nan;
 	PrimeNumber* factorlist = malloc(sizeof(PrimeNumber));
	if(factorlist == NULL)
 		exit(EXIT_FAILURE);
	factorlist->nbr=0;
	factorlist->multiple=0;
	factorlist->file="x";
 	nan = readfrombuffer();
 	while(nan->nombre!=0) {
 		decomp(nan->nombre, factorlist, nan->nomfichier);
 		free(nan);
 		nan = readfrombuffer();
 	}
	return factorlist;
}


/*
 * Décomposition en facteurs premiers d'un nombre
 *
 * @nbr : Nombre à factoriser
 * @list : Liste chainée de PrimeNumber à laquelle il faut ajouter les facteurs premiers trouvés
 * @filename : Nom du fichier dans lequel nbr apparait, cela permet d'afficher le nom du fichier lorsque le facteur premier n'apparaissant qu'une fois a été trouvé
 */
void decomp(uint64_t nbr, PrimeNumber* list, char* filename){

	uint64_t factor = pollard(nbr);
	if (factor == 0){ //La méthode de pollard n'étant pas infaillible, si aucun facteur n'a été trouvé, on vérifiera que le nombre est bien premier en utilisant la méthode naïve
	
		factor = fact(nbr);
		
		if (factor == 0){ //Si la méthode naïve ne trouve pas non plus de facteur, le nombre est donc bien premier...
			addprimefactor(nbr, list, filename); //... et peut être ajouté à la liste
			}
		else {
			decomp(factor, list, filename); //Si un diviseur est trouvé, on fait un appel récursif sur ce diviseur et le résultat de la division
			decomp(nbr/factor, list, filename);
		}
	}
	else {
		decomp(factor, list, filename);
		decomp(nbr/factor, list, filename);
	}
}


	
/*
 * pollard
 * Cherche un facteur d'un nombre donné en utilisant la méthode Rho de Pollard
 * Attention : Cette méthode est plus rapide que la méthode naïve mais n'est pas infaillible
 *
 * @nbr : Nombre dont il faut trouver un facteur
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
	
	for(i=0; d != nbr ; i++){ //La condition d!=nbr est arbitraire et permet uniquement d'éviter un livelock si le nombre est premier ou si aucun facteur n'est trouvé. Si un facteur n'est pas trouvé assez vite, on utilisera la méthode naïve pour vérifier que ce nombre est bien premier.
	
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
	
	if(nbr == 2)
		return 0;
	
	if (nbr % 2 == 0) //Vérifie que 2 n'est pas un facteur
		return 2;
	
	for(i=3; i < nbr; i=i+2){ //Ne teste pas les nombres pairs car, puisque 2 n'est pas un facteur, ces multiples ne peuvent pas non plus en être
	
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
void addprimefactor(uint64_t factor64, PrimeNumber* factorlist, char* filename){

	uint32_t factor = factor64 & 0xFFFFFFFF; //Transforme le facteur, qui est initialement stocké sur 64 bits en format uint32_t
	
	if((*factorlist).nbr == 0){ //Cas où la liste est vide
	
		(*factorlist).nbr = factor;
		(*factorlist).multiple = 0;
		(*factorlist).file = filename;
		(*factorlist).next = NULL;
				
	}
	else { //La liste contient au moins un élément
		PrimeNumber *previousprime;
		PrimeNumber *currentprime = factorlist;
		
		if(factor < (*currentprime).nbr){ //Le nombre premier doit être ajouté en tête de liste
			PrimeNumber* newprime = (PrimeNumber*) malloc(sizeof(PrimeNumber));
			if(newprime == NULL)
				exit(EXIT_FAILURE);
			(*newprime).nbr = factor;
			(*newprime).multiple = 0;
			(*newprime).file = filename;
			newprime -> next = factorlist;
			factorlist = newprime;
			
		}
		else{
		
			while((*currentprime).nbr < factor && currentprime -> next != NULL){ //On parcourt la liste jusqu'à trouver l'emplacement ou la nombre doit être ajouté
				previousprime = currentprime;
				currentprime = currentprime -> next;
			}
			if((*currentprime).nbr == factor){
				(*currentprime).multiple = 1; //Si le nombre se trouve déja dans la liste, on modifie simplement sa variable "multiple" pour indiquer qu'il apparait plus d'une fois 
			}
			else if((*currentprime).nbr > factor) {
				PrimeNumber* newprime = (PrimeNumber*) malloc(sizeof(PrimeNumber));
				if(newprime == NULL)
					exit(EXIT_FAILURE);
				(*newprime).nbr = factor;
				(*newprime).multiple = 0;
				(*newprime).file = filename;
				newprime -> next = currentprime;
				previousprime -> next = newprime;
			}
			else {
				PrimeNumber* newprime = (PrimeNumber*) malloc(sizeof(PrimeNumber));
				if(newprime == NULL)
					exit(EXIT_FAILURE);
				(*newprime).nbr = factor;
				(*newprime).multiple = 0;
				(*newprime).file = filename;
				newprime -> next = NULL;
				currentprime -> next = newprime;
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
	int i;
	for(i=1; i<argc; i++){
		if(strcmp(argv[i], "-stdin")==0 || strcmp(argv[i],"file")==0 || strncmp(argv[i],"http://",7)==0)
			files++;
	}
	return files;
}

/*
 * readfile
 * lit un fichier dont le nom est spécifié par filename c, et place les nombres provenant de ce fichier dans le buffer
 *
 * filename : nom du fichier à lire
 */
void * readfile(void * filename){
	char * nomfichier = (char *) filename;
	int descr;
	int e;
	uint64_t nombre;
	numberandname * nan;
	//ouvrir le fichier
	if((descr=open(filename, O_RDONLY))<0){
		perror("open");
		return NULL;
	}
	//lire le fichier
	while( (e=read(descr, &nombre, sizeof(uint64_t))) != 0){
		if(e<0){
			perror("read");
			return NULL;
		}
		nan=(numberandname *)malloc(sizeof(numberandname));
		if(nan==NULL){
			return NULL;
		}
		nan->nombre=be64toh(nombre);//nombre transformé en représentation locale
		nan->nomfichier=filename;
		addtobuffer(nan);
	}
	//fermer le fichier
	if(close(descr)!=0){
		perror("close");
		return NULL;
	}
}

/*
 * readURL
 * lit un fichier url dont l'adresse est spécifiée par urlname, et place les nombres provenant de ce fichier dans le buffer
 *
 * urlname : nom du fichier URL à lire
 */
void * readURL(void * urlname){
/*
	char * nomurl=(char *) urlname;
	int descr;
	int e;
	uint64_t nombre;
	numberandname * nan;
	//ouvrir le fichier depuis le reseau
	const char * mode="r";
	URL_FILE *url =url_fopen(nomurl,mode);
	if(url==NULL){
		perror("openurl");
		return NULL;
	}
	//lire le fichier depuis le reseau
	while(url_feof(url)==0){//tant que le fichier n'est pas terminé
		if((int)url_fread(&nombre, sizeof(uint64_t), 1, url)==0){
			perror("readurl");
			return NULL;
		}
		nan=(numberandname *)malloc(sizeof(numberandname));
		if(nan==NULL){
			return NULL;
		}
		nan->nombre=be64toh(nombre);
		nan->nomfichier=nomurl;
		addtobuffer(nan);
	}
	//fermer le fichier
	if(url_fclose(url)!=0){
		perror("closeurl");
		return NULL;
	}
*/
return NULL;
}


/*
 * readstdin
 * lit l'entrée standard et place les nombres provenant de l'entrée standard dans le buffer
 *
 * arg : inutilisé (==NULL)
 */
void * readstdin(void * arg){
	int descr;
	int e;
	uint64_t nombre;
	numberandname * nan;
	while( (e=read(0, &nombre, sizeof(uint64_t))) != 0){
		if(e<0){
			perror("readstdin");				
			return NULL;
		}
		nan=(numberandname *)malloc(sizeof(numberandname));
		if(nan==NULL){
			return NULL;
		}
		nan->nombre=be64toh(nombre);//nombre transformé en représentation locale
		nan->nomfichier="stdin";
		addtobuffer(nan);
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
		ind++;
		buffer[ind]=nan;
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
		if(ind==-1 && finishedreading==1){
			sem_post(&full);
			nan=(numberandname *)malloc(sizeof(numberandname));
			if(nan==NULL){
				return NULL; // compléter
			}
			nan->nombre=0;
			nan->nomfichier="x";
		}
		else{
			nan=buffer[ind];
			ind--;
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
					nextaddnode=addnode->next;
					free(addnode);
					addnode=NULL;
					addnode=nextaddnode;
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


/*
 * freelikedlist
 * libere la liste chainée des facteurs premiers
 * 
 * finallist : liste chainée finale contenant tous les facteurs premiers trouvé dans les fichiers
 */
void freelinkedlist(PrimeNumber * finallist){
	PrimeNumber * next;
	while(finallist!=NULL){
		next=finallist->next;
		free(finallist);
		finallist=NULL;
		finallist=next;
	}
}
