#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <endian.h>

int main(int argc, char * argv[]){
	uint64_t nombre=1;
	uint64_t nbr;
	char * newfile= "file5";
	FILE * file;
	int e;
	//ouvrir le fichier
	file=fopen(newfile, "w");
	//écrire le fichier
	int i;
	for(i=0; i<100000; i++){
		nbr=htobe64(nombre);
		e=(int)fwrite(&nbr, sizeof(uint64_t), 1, file);
		if(e<=0){
			perror("write");
			return EXIT_FAILURE;
		}
		nombre=nombre+2;
	}
	//fermer le fichier
	if(fclose(file)!=0){
		perror("close");
		return EXIT_FAILURE;
	}
}
