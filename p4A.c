/** p4A.c                         Ubaldo, Fernando M */

/* Proceso independiente A
 */

#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

int main(void) {
	sem_t *semA;
    sem_t *semD;
	semA=sem_open("semA",0);
    semD=sem_open("semD",0);
	int i=0;
    while(i<40) {
		sem_wait(semA);
		printf("A\n");
		sleep(1);
		sem_post(semD);
		i++;
	}
}
