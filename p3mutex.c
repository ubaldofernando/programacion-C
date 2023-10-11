/** p3mutex.c                           Ubaldo, Fernando M */

/* Implementación de una sincronización de los hilos A, B y C de forma tal, que la secuencia
 * de ejecución y acceso a su sección crítica sea la siguiente: ABAC... detener el proceso luego
 * de 20 iteraciones completas. 
 * 
 *  Resolver la sincronización con variables Mutex 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void *hiloA(void *);
void *hiloB(void *);
void *hiloC(void *);

pthread_mutex_t mA = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t mB = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t mC = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t mD = PTHREAD_MUTEX_INITIALIZER; 

int iteraciones=20;

int main(int argc, char **argv) {

	pthread_t thA,thB,thC;
	printf("lock SemC,SemD\n");
	pthread_mutex_lock(&mC);  
	pthread_mutex_lock(&mD);  
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
		pthread_mutex_lock(&mA);  
			printf("A ");
		pthread_mutex_unlock(&mD); 
		i--;
	}
	pthread_exit(NULL);  
}

void *hiloB(void *p) {
	int i=iteraciones;
	while(i>0) {
		pthread_mutex_lock(&mB); 
		pthread_mutex_lock(&mD);  
			printf("B ");
		pthread_mutex_unlock(&mC); 
		pthread_mutex_unlock(&mA); 
		i--;
	}
	pthread_exit(NULL);  
}

void *hiloC(void *p) {
	int i=iteraciones;
	while(i>0) {
		pthread_mutex_lock(&mC);  
		pthread_mutex_lock(&mD);  
			printf("C\n");
			sleep(1);
		pthread_mutex_unlock(&mB); 
		pthread_mutex_unlock(&mA); 
		i--;
	}
	pthread_exit(NULL);  
}
