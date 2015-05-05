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
 * @index : index du nom du fichier d'origine dans argv[]
 */
typedef struct{
	short index; 
	uint64_t nombre;
} numberAndIndex;

/*
 * Liste chainée de facteurs premiers
 *
 * @index : index du nom du fichier d'origine dans argv[]
 * @facteur : Facteur premier
 * @multiple : 0 si le facteur n'apparait qu'une seule fois
 * @next : nombre premier suivant dans la liste chainée
 */
typedef struct primeNumber{
	short index;
	uint32_t facteur; 
	int multiple; 
	struct primeNumber * next;
} primeNumber;


/*
 * un nom de fichier et son index dans argv[]
 *
 * @index : index du nom du fichier d'origine dans argv[]
 * @file : nom du fichier
 */
typedef struct {
	short index;
	char * file;
} fileAndIndex;


uint64_t pollard(uint64_t);
void addprimefactor(uint32_t, primeNumber*, short);
void * factorise(void *);
void decomp(uint64_t, primeNumber*, short);
uint64_t pollard(uint64_t);
uint64_t naive(uint64_t);
uint64_t gcd(uint64_t, uint64_t);

int getmaxthreads(int argc, char * argv[]);
int filescount (int argc, char * argv[]);
void * readfile(void * filename);
void * readURL(void * urlname);
void * readstdin(void * arg);
void addtobuffer(numberAndIndex * nan);
numberAndIndex * readfrombuffer();
primeNumber * merge(int nthr, primeNumber * retvals[]);
primeNumber * findsolution(primeNumber * finallist);
void freelinkedlist(primeNumber * finallist);

numberAndIndex * buffer[BSIZE];
int curr=-1; //index du prochain nombre du buffer à factoriser
pthread_mutex_t buffermutex;	
sem_t empty;
sem_t full;

//___________________________________________________________________________

int main(int argc, char * argv[]){
	struct timeval tv1;
	struct timeval tv2;
	gettimeofday(&tv1, NULL);

	//récupérer le nombre max de threads passé en argument, qui vaut le nombre de threads consommateurs utilisés
	int consnumber=getmaxthreads(argc, argv);
	if(consnumber==0)
		return EXIT_FAILURE;
	
	//initialisation des mutex/sémaphores associées au buffer à partager entre producers et consumers
	pthread_mutex_init( &buffermutex, NULL);	
	sem_init(&empty, 0, BSIZE);
	sem_init(&full, 0, 0);

	//déterminer le nombre de fichiers à lire pour savoir combien de threads producteur il va falloir lancer
	int prodnumber=filescount (argc, argv);

	curl_global_init(CURL_GLOBAL_ALL);//pour permettre une utilisation propre de curl dans la lecture des URLs

	//lancer la lecture des fichiers pour alimenter le buffer, 1 fichier = 1 thread
	pthread_t threadsprod [prodnumber];
	fileAndIndex * fax;
	int a=0;
	int i;
	for(i=0; i<argc; i++){
		if(strcmp(argv[i], "-stdin")==0){
			fax = (fileAndIndex *)malloc(sizeof(fileAndIndex));
			fax->index=(short)i;
			fax->file="-stdin";
			pthread_create(&(threadsprod[a]), NULL, &readstdin, (void *)fax);
			a++;
		}			
		else if (strncmp(argv[i],"file",4)==0){
			fax = (fileAndIndex *)malloc(sizeof(fileAndIndex));
			fax->index=(short)i;
			fax->file=argv[i];
			pthread_create(&(threadsprod[a]), NULL, &readfile, (void *)fax); 
			a++;
		}
		else if (strncmp(argv[i],"http://",7)==0){
			fax = (fileAndIndex *)malloc(sizeof(fileAndIndex));
			fax->index=(short)i;
			fax->file=argv[i];
			pthread_create(&(threadsprod[a]), NULL, &readURL, (void *)fax);
			a++; 
		}
	}

	//lancer les threads consumer pour factoriser les nombres
	pthread_t threadscons [consnumber];
	for(i=0; i<consnumber; i++){
		pthread_create(&(threadscons[i]), NULL, &factorise, NULL);
	}

	//on récuppère les threads producteurs
	for(i=0; i<prodnumber; i++){
		pthread_join(threadsprod[i], NULL);
	}

	curl_global_cleanup();

	sem_post(&full);//pour débloquer les threads consumers (similaire à problème du rdv)

	//récupère les listes chainées de chaque thread
	primeNumber * retvals [consnumber]; 
	for(i=0; i<consnumber; i++){
		pthread_join(threadscons[i], (void **)&(retvals[i]));
	}

	//merge les listes chainées
	primeNumber * finallist = merge(consnumber, retvals);	

	//trouve le résultat, et printf
	primeNumber * solution = findsolution(finallist);
	if(solution==NULL){
		perror("no solution found");
		return EXIT_FAILURE;
	}

	printf("solution : %u\n", solution->facteur);
	printf("fichier : %s\n", argv[solution->index]);

	//free finallist
	freelinkedlist(finallist);

	//temps écoulé
	gettimeofday(&tv2, NULL);
	
	int sec = (int)(tv2.tv_sec - tv1.tv_sec);
	int usec = (int)(tv2.tv_usec - tv1.tv_usec);
	if(usec < 0){
		sec = sec-1;
		usec = 1000000+usec;
	}
	printf("sec écoulé : %d\n", sec);
	printf("usec écoulé : %d\n", usec);
	return EXIT_SUCCESS;
}
	



/*
 * factorise
 *
 * fonction de départ des threads consommateurs, factorise les nombres premiers du buffer, et renvoie une liste chainée des
 * facteurs premiers qu'elle a trouvée
 */
void * factorise(void * arg){
 	numberAndIndex* nai;
 	primeNumber* factorlist =(primeNumber *) malloc(sizeof(primeNumber));
	if(factorlist == NULL)
 		exit(EXIT_FAILURE);
	factorlist->facteur=0;
	factorlist->multiple=1;
	factorlist->index=0;
	factorlist -> next = NULL;
 	nai = readfrombuffer();
 	while(nai->nombre!=0) {
 		decomp(nai->nombre, factorlist, nai->index);
 		free(nai);
 		nai = readfrombuffer();
 	}
	free(nai);
	return factorlist;
}



/*
 * Décomposition en facteurs premiers d'un nombre
 *
 * @facteur : Nombre à factoriser
 * @list : Liste chainée de primeNumber à laquelle il faut ajouter les facteurs premiers trouvés
 * @index : Nom du fichier dans lequel facteur apparait, cela permet d'afficher le nom du fichier lorsque le facteur premier n'apparaissant qu'une fois a été trouvé
 */
void decomp(uint64_t facteur, primeNumber* list, short index){
	uint64_t factor = pollard(facteur);
	if (factor == 0){ //La méthode de pollard n'étant pas infaillible, si aucun facteur n'a été trouvé, on vérifiera que le nombre est bien premier en utilisant la méthode naïve
		factor = naive(facteur);
		if(factor == 0) //Si la méthode naïve ne trouve pas non plus de facteur, le nombre est donc bien premier...
			addprimefactor((uint32_t)facteur, list, index); //... et peut être ajouté à la liste
		else{
			decomp(factor, list, index); //Si un diviseur est trouvé, on fait un appel récursif sur ce diviseur et le résultat de la division
			decomp(facteur/factor, list, index);
		}
	}
	else{
		decomp(factor, list, index);
		decomp(facteur/factor, list, index);
	}
}


	
/*
 * pollard
 * Cherche un facteur d'un nombre donné en utilisant la méthode Rho de Pollard
 * Attention : Cette méthode est plus rapide que la méthode naïve mais n'est pas infaillible
 *
 * @facteur : Nombre dont il faut trouver un facteur
 * @return : Facteur de n trouvé, ou 0 si aucun facteur n'a été trouvé
 */
uint64_t pollard(uint64_t facteur){
	if(facteur==1)
		return 0;
	int i;	
	uint64_t x = rand() % (facteur+1); //Choisi un nombre aléatoirement entre 0 et n
	uint64_t y = x;
	int k = 2;
	uint64_t d = 0;
	for(i=0; d != facteur ; i++){ //La condition d!=facteur est arbitraire et permet uniquement d'éviter un livelock si le nombre est premier ou si aucun facteur n'est trouvé. Si un facteur n'est pas trouvé assez vite, on utilisera la méthode naïve pour vérifier que ce nombre est bien premier.
		x = f(x) % facteur;
		d = gcd(abs(y-x), facteur);
		if((d != 1) && (d != facteur)){
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
    while (b != 0){
        c = a % b;
        a = b;
        b = c;
    }
    return a;
}
 


/*
 * naive
 * Cherche un facteur premier d'un nombre donné en utilisant la méthode naïve
 *
 * @facteur : Nombre dont il faut trouver un facteur
 * @return : Premier facteur trouvé, ou 0 si aucun facteur n'a été trouvé (facteur est donc premier)
 */ 
uint64_t naive(uint64_t facteur){
	uint64_t i;
	if(facteur == 2)
		return 0;
	if (facteur % 2 == 0) //Vérifie que 2 n'est pas un facteur
		return 2;
	for(i=3; (i*i) <= facteur; i=i+2){ //Ne teste pas les nombres pairs car, puisque 2 n'est pas un facteur, ces multiples ne peuvent pas non plus en être. On s'arrête à la racine carée de facteur car aucun nombre supérieur à celle-ci ne pourra être un diviseur de facteur.
		if (facteur % i == 0)
			return i; //Retourne i si c'est un diviseur de facteur
	}
	return 0; //Aucun diviseur n'a été trouvé, facteur est donc premier
}



/*
 * addprimefactor
 * Ajoute un facteur premier à la liste chainée triée contenant les facteurs
 * Si ce facteur ce trouve déja dans la liste, il change la variable "multiple" de la structure contenant le facteur
 *
 * @factor64 : Facteur premier à ajouter à la liste sous la forme uint64_t
 * @factorlist : Pointeur vers les pointeur du premier élément de la liste
 * @index : fichier duquel le facteur premier est tiré
 */
void addprimefactor(uint32_t factor, primeNumber* factorlist, short index){
	primeNumber *previousprime;
	primeNumber *currentprime = factorlist;		

	while(currentprime->facteur < factor && currentprime -> next != NULL){ //On parcourt la liste jusqu'à trouver l'emplacement ou la nombre doit être ajouté
		previousprime = currentprime;
		currentprime = currentprime -> next;
	}
	if(currentprime->facteur == factor){
		currentprime->multiple = 1; //Si le nombre se trouve déja dans la liste, on modifie simplement sa variable "multiple" pour indiquer qu'il apparait plus d'une fois 
	}
	else if(currentprime->facteur > factor) {// s'il faut ajouter le facteur entre previous et currentprime
		primeNumber* newprime = (primeNumber*) malloc(sizeof(primeNumber));
		if(newprime == NULL)
			exit(EXIT_FAILURE);
		newprime->facteur = factor;
		newprime->multiple = 0;
		newprime->index = index;
		newprime -> next = currentprime;
		previousprime -> next = newprime;
	}
	else {// s'il faut ajouter le facteur en fin de liste
		primeNumber* newprime = (primeNumber*) malloc(sizeof(primeNumber));
		if(newprime == NULL)
			exit(EXIT_FAILURE);
		newprime->facteur = factor;
		newprime->multiple = 0;
		newprime->index = index;
		newprime -> next = NULL;
		currentprime -> next = newprime;
	}
}
	

/*
 * getmaxthread
 * a partir de la ligne de commande, renvoie le nombre max de threads utilisables donnee
 *
 * argc : argc de main
 * argv : argv de main
 * return = le nombre max de threads utilisables par le programme
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
 * return = le nombre de fichiers à lire par le programme
 */
int filescount (int argc, char * argv[]){
	int files=0;
	int i;
	for(i=1; i<argc; i++){
		if(strcmp(argv[i], "-stdin")==0 || strncmp(argv[i],"file",4)==0 || strncmp(argv[i],"http://",7)==0)
			files++;
	}
	return files;
}



/*
 * readfile
 * lit un fichier dont le nom est spécifié par filename c, et place les nombres provenant de ce fichier dans le buffer
 *
 * filename : nom du fichier à lire
 * return = NULL
 */
void * readfile(void * arg){
	fileAndIndex * fax=(fileAndIndex *) arg;
	char * file = fax->file;
	short index=fax->index;
	free(fax);
	fax=NULL;
	int descr;
	int e;
	uint64_t nombre;
	numberAndIndex * nai;
	//ouvrir le fichier
	if((descr=open(file, O_RDONLY))<0){
		perror("open");
		exit(EXIT_FAILURE);
	}
	//lire le fichier
	while( (e=read(descr, &nombre, sizeof(uint64_t))) != 0){
		if(e<0){
			perror("read");
			exit(EXIT_FAILURE);
		}
		nai=(numberAndIndex *)malloc(sizeof(numberAndIndex));
		if(nai==NULL){
			exit(EXIT_FAILURE);
		}
		nai->nombre=be64toh(nombre);//nombre transformé en représentation locale
		nai->index=index;
		addtobuffer(nai);
	}
	//fermer le fichier
	if(close(descr)!=0){
		perror("close");
		exit(EXIT_FAILURE);
	}
}



/*
 * readURL
 * lit un fichier url dont l'adresse est spécifiée par urlname, et place les nombres provenant de ce fichier dans le buffer
 *
 * urlname : nom du fichier URL à lire
 * return = NULL
 */
void * readURL(void * arg){
	fileAndIndex * fax=(fileAndIndex *) arg;
	char * file = fax->file;
	short index=fax->index;
	free(fax);
	fax=NULL;
	int descr;
	int e;
	uint64_t nombre;
	numberAndIndex * nai;
	//ouvrir le fichier depuis le reseau
	const char * mode="r";
	URL_FILE *url =url_fopen(file,mode);
	if(url==NULL){
		perror("openurl");
		exit(EXIT_FAILURE);
	}

	//lire le fichier depuis le reseau
	while(url_feof(url)==0){//tant que le fichier n'est pas terminé
		if((int)url_fread(&nombre, sizeof(uint64_t), 1, url)==0){
			perror("readurl");
			exit(EXIT_FAILURE);
		}
		nai=(numberAndIndex *)malloc(sizeof(numberAndIndex));
		if(nai==NULL){
			exit(EXIT_FAILURE);
		}

		nai->nombre=be64toh(nombre);
		nai->index=index;
		addtobuffer(nai);
	}

	//fermer le fichier
	if(url_fclose(url)!=0){
		perror("closeurl");
		exit(EXIT_FAILURE);
	}
	return NULL;
}



/*
 * readstdin
 * lit l'entrée standard et place les nombres provenant de l'entrée standard dans le buffer
 *
 * arg : inutilisé (==NULL)
 * return = NULL
 */
void * readstdin(void * arg){
	fileAndIndex * fax=(fileAndIndex *) arg;
	short index=fax->index;
	free(fax);
	fax=NULL;
	int descr;
	int e;
	uint64_t nombre;
	numberAndIndex * nai;
	while( (e=read(0, &nombre, sizeof(uint64_t))) != 0){
		if(e<0){
			perror("readstdin");				
			exit(EXIT_FAILURE);
		}
		nai=(numberAndIndex *)malloc(sizeof(numberAndIndex));
		if(nai==NULL){
			exit(EXIT_FAILURE);
		}
		nai->nombre=be64toh(nombre);//nombre transformé en représentation locale
		nai->index=index;
		addtobuffer(nai);
	}
	return NULL;
}



/*
 * addtobuffer
 * rajoute le numberAndIndex passé en argument à buffer en respectant le protocole des producers-consumers
 *
 * nan : structure numberAndIndex a ajouter au buffer
 *
 */
void addtobuffer(numberAndIndex * nan){
	sem_wait(&empty);
	pthread_mutex_lock(&buffermutex);
		curr++;
		buffer[curr]=nan;
	pthread_mutex_unlock(&buffermutex);
	sem_post(&full);	
}



/*
 * readfrombuffer
 * retire et renvoie un numberAndIndex de buffer en respectant le protocole des producers-consumers
 * renvoie un pointeur vers un numberAndIndex avec un nombre=0 si le buffer est vide est la lecture de tous les fichiers est terminée
 * 
 * return = une structure numberAndIndex provenant du buffer
 */
numberAndIndex * readfrombuffer(){
	numberAndIndex * nan;
	sem_wait(&full);
	pthread_mutex_lock(&buffermutex);
		if(curr==-1){//cette condition ne sera validée que quand tous les threads consommateurs doivent être terminés
			sem_post(&full);
			nan=(numberAndIndex *)malloc(sizeof(numberAndIndex));
			if(nan==NULL){
				return NULL;
			}
			nan->nombre=0;
			nan->index=0;
		}
		else{
			nan=buffer[curr];
			curr--;
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
 * return = la liste chainée résultante contenant tous les facteurs premiers rencontrés dans les fichiers
 */
primeNumber * merge(int nthr, primeNumber * retvals[]){
	primeNumber * finallist=(primeNumber *)malloc(sizeof(primeNumber));
	if(finallist==NULL){
		return NULL;
	}
	finallist->facteur=0;
	finallist->multiple=1;
	finallist->index=0;
	finallist->next=retvals[0];
	// on prend la liste chainée du premier thread, puis on y rajoute succésivement les listes chainées des autres threads.
	primeNumber * currentnode;
	primeNumber * nextnode;
	primeNumber * addnode;
	primeNumber * nextaddnode;
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
				if(addnode->facteur == nextnode->facteur){//si le nombre a insérer est déjà dans la liste principale
					nextnode->multiple=1;
					nextaddnode=addnode->next;
					free(addnode);
					addnode=NULL;
					addnode=nextaddnode;
				}
				else if(addnode->facteur < nextnode->facteur){//s'il faut insérer addnode entre currentnode et nextnode
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
 * return = le nombre premier d'occurence unique le plus élevé trouvé dans la liste finale
 */
primeNumber * findsolution(primeNumber* finallist){
	primeNumber * solution=finallist;//parce qu'on sait que le premier élément de finalist est 0
	while(finallist!=NULL){
		if(finallist->multiple==0){
			return finallist;
		}
		finallist=finallist->next;
	}
	return NULL;
}



/*
 * freelikedlist
 * libere la liste chainée des facteurs premiers
 * 
 * finallist : liste chainée finale contenant tous les facteurs premiers trouvé dans les fichiers
 */
void freelinkedlist(primeNumber * finallist){
	primeNumber * next;
	while(finallist!=NULL){
		next=finallist->next;
		free(finallist);
		finallist=NULL;
		finallist=next;
	}
}
