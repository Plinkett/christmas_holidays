#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<dirent.h> //Contiene DIR, opendir(), dirdf() e closedir()
#include<unistd.h> 
#include<sys/types.h>

#define MAX 20

int main(){
	char directory[MAX];
	char percorsoRelativo[MAX];
	DIR *flussoDirectory; 
	int fddir;
	int fdfile;

	printf("Inserisci la directory iniziale: \n");
	scanf("%s", directory);
	
	flussoDirectory = opendir(directory);
	if(flussoDirectory == NULL){
		perror("Directory non valida ");
		exit(1);
	}

	fddir = dirfd(flussoDirectory);
	if(fddir == -1){
		perror("Errore nell'estrazione del file descriptor ");
		exit(1);
	}
	//flussoDirectory non deve essere chiuso

	printf("Il file descriptor della directory e' %d \n", fddir);
	
	printf("Inserisci il percorso relativo: \n");
	scanf("%s", percorsoRelativo);

	fdfile = openat(fddir, percorsoRelativo, O_RDWR|O_CREAT, 0666);
	
	closedir(flussoDirectory);

	if(fdfile == -1){
		perror("Errore nella lettura del file ");
		exit(1);
	}
	else{
		printf("Il file e' stato aperto con successo! \n");	
		close(fdfile);
	}
}
