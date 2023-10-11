/** p6crear.c                         Ubaldo, Fernando M */

/* Implementación de una sincronización con procesos independientes A, 
 * B y C de forma tal, que la secuencia de ejecución y acceso a su 
 * sección crítica sea la siguiente: BACA... detener el proceso luego
 * de 20 iteraciones completas.
 * 
 * Resolver la sincronización con semáforos Posix con nombre.
 */
 
 /** Para resolver este punto se incluyen p4A.c , p4B.c , p4C.c , que son los
 * procesos independientes A, B y C respectivamente.
 * Esto se debe a que la secuencia BACA se consigue con la inicialización 
 * de los semáforos: A=0, B=1, C=0 y D=1, y siendo la orden de finalizacion 
 * de los procesos la siguiente ->  1° p4B, 2° p4C, y 3° p4A.
 * (p4c indica en que iteracion se encuentra)
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
	int ValsemA;
	int ValsemB;
	int ValsemC;
	int ValsemD;
	sem_unlink("semA");
	sem_unlink("semB");
	sem_unlink("semC");
	sem_unlink("semD");
	semA=sem_open("semA",O_CREAT|O_EXCL,0644,0);
	semB=sem_open("semB",O_CREAT|O_EXCL,0644,1);
	semC=sem_open("semC",O_CREAT|O_EXCL,0644,0);
	semD=sem_open("semD",O_CREAT|O_EXCL,0644,1);
	sem_getvalue(semA,&ValsemA);
	sem_getvalue(semB,&ValsemB);
	sem_getvalue(semC,&ValsemC);
	sem_getvalue(semD,&ValsemD);
	printf("semA = %d \n",ValsemA);
	printf("semB = %d \n",ValsemB);
	printf("semC = %d \n",ValsemC);
	printf("semD = %d \n",ValsemD);
}
