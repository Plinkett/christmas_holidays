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

	/* Creazione della memoria condivisa */
	shmid = shmget(chiave_mem, sizeof(char), IPC_CREAT|0660);
	if(shmid == -1){
		perror("Allocazione della memoria condivisa fallita \n");
		exit(1);
	}
	
	/* Associazione della memoria del processo alla memoria condivisa */
	inizio_memoria = shmat(shmid, 0, 0);
	if(inizio_memoria == (char *)-1){	
		perror("Errore shmat \n");
		shmctl(chiave_mem, IPC_RMID, NULL);
		exit(1);
	}
	
	printf("Cella di memoria condivisa %p \n", inizio_memoria);
	/* Creazione del semaforo */
	semid = semget(chiave_sem, 3, IPC_CREAT|0660); 
	if(semid == -1){
		perror("Errore semget \n");
		shmctl(chiave_mem, IPC_RMID, NULL);
		exit(1);
	}
	
	/* Setto tutti i semafori ai valori indicati da arg.array.
	   In questo caso esistono 3 semafori */
	arg.array = malloc(sizeof(int) * 3);
	*(arg.array) = 	1; /* buffer vuoto */
	*(arg.array + 1) = 0; /* buffer vuoto */
	*(arg.array + 2) = 0; /* client */
	if(semctl(semid, 0, SETALL, arg.array) == -1){ 	
		perror("Errore semctl \n");
		shmdt(inizio_memoria);
		semctl(chiave_sem, 0, IPC_RMID);
		shmctl(chiave_mem, IPC_RMID, NULL);
		exit(1);
	}

	printf("Dopo SETALL \n");
	printf("Primo semaforo: %d \n", semctl(semid, 0, GETVAL));
	printf("Secondo semaforo: %d \n", semctl(semid, 1, GETVAL));
	printf("Terzo semaforo %d \n", semctl(semid, 2, GETVAL));

	op[0].sem_num = 0; /* semaforo bufferVuoto */
	op[0].sem_op = -1;
	op[0].sem_flg = SEM_UNDO;

	op[1].sem_num = 1; /* semaforo bufferPieno*/
	op[1].sem_op = 1;
	op[1].sem_flg = SEM_UNDO;
	
	op[2].sem_num = 2; /*semaforo che indica che il client ha finito e quindi posso eliminare il semaforo e la memoria condivisa*/
	op[2].sem_op = -1;
	op[2].sem_flg = SEM_UNDO;

	printf("\n Ciclo for \n");
	for(i = 0; i < 4; i++){
		/* wait(bufferVuoto)*/
		if(semop(semid, op, 1) == -1){
			perror("wait non effettuata \n");
			shmdt(inizio_memoria);
			shmctl(chiave_mem, IPC_RMID, NULL);
			semctl(chiave_sem, 0, IPC_RMID);
			exit(1);
		}
		
		/* Sezione critica */
			printf("Primo semaforo: %d \n", semctl(semid, 0, GETVAL));
			printf("Secondo semaforo: %d \n", semctl(semid, 1, GETVAL));
			/*Nella sezione critica valgono sempre zero*/
			*inizio_memoria = 'a' + i;	
			printf("Ho generato il carattere numero %d \n\n", i);
			sleep(1);
		/* Fine sezione critica */
		
		/* signal(bufferPieno) */
		if(semop(semid, op + 1, 1) == -1){
			perror("signal non effettuata \n");			
			shmdt(inizio_memoria);
			semctl(chiave_sem, 0, IPC_RMID);
			shmctl(chiave_mem, IPC_RMID, NULL);
			exit(1);
		}
	}

	printf("Dopo il ciclo for \n");
	printf("Primo semaforo: %d \n", semctl(semid, 0, GETVAL));
	printf("Secondo semaforo: %d \n", semctl(semid, 1, GETVAL));
	printf("Terzo semaforo: %d \n", semctl(semid, 2, GETVAL));
	/*wait(client)*/
	if(semop(semid, op + 2, 1) == -1){
		perror("signal finale non effettuata \n");
		exit(1);	
	}

	shmdt(inizio_memoria);
	shmctl(shmid, IPC_RMID, NULL);
	semctl(semid, 0, IPC_RMID);
	
	printf("Fine del programma \n");
	
}   	
