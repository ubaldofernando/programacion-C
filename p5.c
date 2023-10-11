/** p5.c                         Ubaldo, Fernando M */

/* Implementación de una sincronización con procesos emparentados PadreA,
 * HijoB y HijoC de forma tal, que la secuencia de ejecución y acceso a 
 * su sección crítica sea la siguiente: PadreAHijoBPadreAHijoC... detener
 * el proceso luego de 20 iteraciones completas. 
 * 
 * Resolver la sincronización con semáforos Posix con nombre.
 */
 
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

int main(void) {
	sem_t *semA;
    sem_t *semB;
    sem_t *semC;
    sem_t *semD;
    int ValsemA;
    int ValsemB;
    int ValsemC;
    int ValsemD;
	sem_unlink("/semA");
	sem_unlink("/semB");
	sem_unlink("/semC");
	sem_unlink("/semD");
	semA=sem_open("/semA",O_CREAT|O_EXCL,0644,1);
	semB=sem_open("/semB",O_CREAT|O_EXCL,0644,1);
	semC=sem_open("/semC",O_CREAT|O_EXCL,0644,0);
	semD=sem_open("/semD",O_CREAT|O_EXCL,0644,0);
	sem_getvalue(semA,&ValsemA);
	sem_getvalue(semB,&ValsemB);
	sem_getvalue(semC,&ValsemC);
	sem_getvalue(semD,&ValsemD);
	printf("semA = %d \n",ValsemA);
	printf("semB = %d \n",ValsemB);
	printf("semC = %d \n",ValsemC);
	printf("semD = %d \n \n",ValsemD);
	int c=0;
	int t=20;
	pid_t pid;
	pid=fork();				
	if (pid != 0) {
		// estoy en el proceso padre: Padre A
		pid=fork();
		if (pid != 0) {
			t=t*2;
			while(t != 0) {
				sem_wait(semA);
				printf("Padre A \n");
				sleep(1);
				sem_post(semD);
				t--;
			}
		} else {
			//soy uno de los hijos: Hijo C
			while(t != 0) {
				sem_wait(semC);
				sem_wait(semD);
				printf("Hijo C\n       iteración = %d\n", ++c);
				sleep(1);
				sem_post(semA);
				sem_post(semB);
				t--;
			} 
		}
	} else {
		//soy uno de los hijos: HijoB
		while(t != 0) {
			sem_wait(semB);
			sem_wait(semD);
			printf("Hijo B \n");
			sleep(1);
			sem_post(semA);
			sem_post(semC);
			t--;
		}
	}
}
	
