#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>

int main(int argc, char * argv[]){
	uint64_t nombre=1;
	char * newfile= "file5";
	FILE * file;
	int e;
	//ouvrir le fichier
	file=fopen(newfile, "w");
	//écrire le fichier
	int i;
	for(i=0; i<10000; i++){
		e=(int)fwrite(&nombre, sizeof(uint64_t), 1, file);
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
