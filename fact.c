#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl\curl.h>
#include <endian.h>

#define f(x)  (x*x+1)

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
	
	
	
//Décomposition en facteurs premiers d'un nombre
void decomp(void* nbr){
		
}
	
/*
 * pollard
 * Cherche un facteur d'un nombre donné en utilisant la méthode Rho de Pollard
 * Attention : Cette méthode est plus rapide que la méthode naïve mais n'est pas infaillible
 *
 * @nbr : Nombre ddont il faut trouver un facteur
 * @return : Facteur de n trouvé, ou -1 si aucun facteur n'a été trouvé
 */
int pollard(int nbr){
	
	if(nbr==1)
		return -1;
	
	int i;	
	int x = rand() % (nbr+1); //Choisi un nombre aléatoirement entre 0 et n
	int y = x;
	int k = 2;
	int d = 0;
	
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
	
	return -1;

}


/*
 * gcd
 * Calcule le PGCD de deux nombres en utilisant l'algorithme d'Euclide
 *
 * @a et @b : Deux nombres dont il faut calculer le PGCD
 * @return : PGCD des deux nombres
 */
int gcd(int a, int b){

    int c;
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
 * @return : Premier facteur trouvé, ou -1 si aucun facteur n'a été trouvé (nbr est donc premier)
 */ 
int fact(int nbr){
	
	int i;
	
	if (nbr % 2 == 0) //Vérifie que 2 n'est pas un facteur
		return 2;
	
	for(i=3; i <= nbr; i=i+2){ //Ne teste pas les nombres pairs car, puisque 2 n'est pas un facteur, ces multiples ne peuvent pas non plus en être
	
		if (nbr % i == 0)
			return i; //Retourne i si c'est un diviseur de nbr
	
	} 
	
	return -1; //Aucun diviseur n'a été trouvé, nbr est donc premier
}
	
//Ajoute un facteur à la liste
	
	
}
