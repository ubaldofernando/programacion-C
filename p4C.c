/** p4C.c                         Ubaldo, Fernando M */

/* Proceso independiente C
 */

#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

int main(void) {
	sem_t *semA;
	sem_t *semB;
	sem_t *semC;   
	sem_t *semD;
	semA=sem_open("semA",0);
	semB=sem_open("semB",0);
	semC=sem_open("semC",0);
	semD=sem_open("semD",0);
	int i=0;
	while(i<20) {
		sem_wait(semC);
		sem_wait(semD);
		printf("C\n          iteración: %d\n",++i);
		sleep(1);
		sem_post(semA);
		sem_post(semB);
	}
}


