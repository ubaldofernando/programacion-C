/** p3posixsn.c                         Ubaldo, Fernando M */

/* Implementación de una sincronización de los hilos A, B y C de forma tal, que la secuencia
 * de ejecución y acceso a su sección crítica sea la siguiente: ABAC... detener el 
 * proceso luego de 20 iteraciones completas
 * 
 * Resolver la sincronización con semáforos Posix sin nombre.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>

void *hiloA(void *);
void *hiloB(void *);
void *hiloC(void *);

sem_t sA;
sem_t sB;
sem_t sC;
sem_t sD;

int iteraciones=20;

int main(int argc, char **argv) {
	
	sem_init(&sA,0,1);
    sem_init(&sB,0,1);
    sem_init(&sC,0,0);
    sem_init(&sD,0,0);     	
	pthread_t thA,thB,thC;
	printf("Lanzo los hilos A,B,C\n");
	pthread_create(&thA,NULL,hiloA,NULL);
	pthread_create(&thB,NULL,hiloB,NULL);
	pthread_create(&thC,NULL,hiloC,NULL);
	pthread_join(thA,NULL);
	pthread_join(thB,NULL);
	pthread_join(thC,NULL);
	printf("Finalizaron los hilos!\n");
	return 0;
}

void *hiloA(void *p) {
	int i=2*iteraciones;
	while(i>0) {
		sem_wait(&sA);
			printf("A ");
		sem_post(&sD); 
		i--;
	}
	pthread_exit(NULL);  
}

void *hiloB(void *p) {
	int i=iteraciones;
	while(i>0) {
		sem_wait(&sB); 
		sem_wait(&sD);  
			printf("B ");
		sem_post(&sC); 
		sem_post(&sA); 
		i--;
	}
	pthread_exit(NULL);  
}

void *hiloC(void *p) {
	int i=iteraciones;
	while(i>0) {
		sem_wait(&sC);  
		sem_wait(&sD); 
			printf("C\n");
			sleep(1);
		sem_post(&sB); 
		sem_post(&sA); 
		i--;
	}
	pthread_exit(NULL);  
}

