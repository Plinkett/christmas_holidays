#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h> /* Per key_t */
#include<sys/ipc.h> /* Per alcuni flag e la funzione ftok() */
#include<sys/sem.h>
#include<sys/shm.h>
#include<unistd.h>

union semun{
	int valore; /* Flag SETVAL */
	unsigned short *array;  /* Per GETALL e SETALL */
	struct semid_ds *buffer; /* Flag IPC_STAT e IPC_SET */
};

int main(){
	int i;
	int semid;
	int shmid;
	char* inizio_memoria;
	key_t chiave_sem;
	key_t chiave_mem;
	union semun arg;
	struct sembuf op[3];

	chiave_sem = ftok(".", 50);
	chiave_mem = ftok(".", 100);

	if(chiave_sem == -1 || chiave_mem == -1){
		perror("Errore ftok \n");
		exit(1);
	}

	shmid = shmget(chiave_mem, sizeof(char), 0660);
	if(shmid == -1){
		perror("Allocazione della memoria condivisa fallita \n");
		exit(1);
	}

	inizio_memoria = shmat(shmid, 0, SHM_RDONLY);
	if(inizio_memoria == (char *)-1){	
		perror("Errore shmat \n");
		exit(1);
	}

	printf("Cella di memoria condivisa %p \n", inizio_memoria);

	semid = semget(chiave_sem, 2, 0660); 
	if(semid == -1){
		perror("Errore semget \n");
		shmdt(inizio_memoria);
		exit(1);
	}
	
	op[0].sem_num = 0; /* semaforo buffer vuoto */
	op[0].sem_op = 1;
	op[0].sem_flg = SEM_UNDO;

	op[1].sem_num = 1; /* semaforo buffer pieno*/
	op[1].sem_op = -1;
	op[1].sem_flg = SEM_UNDO;

	op[2].sem_num = 2; /*semaforo che indica che il client ha finito e quindi posso eliminare il semaforo */
	op[2].sem_op = 1;
	op[2].sem_flg = SEM_UNDO;

	for(i = 0; i < 4; i++){	
		/* wait(bufferPieno) */
		if(semop(semid, op + 1, 1) == -1){
			perror("wait non effettuata \n");			
			exit(1);
		}		
		/* Sezione critica */
			printf("Primo semaforo: %d \n", semctl(semid, 0, GETVAL));
			printf("Secondo semaforo: %d \n", semctl(semid, 1, GETVAL));
			printf("Valore generato dal processo server: %c \n\n", *inizio_memoria);
	
		/* Fine sezione critica */
		
		/* signal(bufferVuoto) */
		if(semop(semid, op, 1) == -1){
			perror("signal non effettuata \n");			
			exit(1);
		}
	}
	
	shmdt(inizio_memoria);
	sleep(2);
	/* signal finale */	
	if(semop(semid, op + 2, 1) == -1){
		perror("signal finale non effettuata \n");
		exit(1);
	}
	printf("Fine del programma \n");
}   	
