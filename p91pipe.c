/** p91pipe.c                         Ubaldo, Fernando M */

/* Realizar un chat entre procesos emparentados usando pipes. Crear una
 * estructura de procesos emparentados de cinco procesos, un proceso
 * padre creará dos procesos hijos y cada proceso hijo creará un proceso
 * nieto. La creación de procesos se realiza con la llamada al sistema 
 * fork. El chat se realizará entre los procesos hijos que serán los 
 * escritores y cada proceso nieto será el lector de su correspondiente
 * escritor. 
 * 
 * 
 * Se decide usar un pipe como recurso de comunicación.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

int main(int argc, char **argv) {
	int pipe1[2];  // pipe1[0] -> lectura  pipe1[1] -> escritura
	pipe(pipe1);
	
	sem_t * semH1 ;
	sem_t * semH2;
	sem_t * semN1;
	sem_t * semN2;
	sem_unlink("/semH1");
	sem_unlink("/semH2");
	sem_unlink("/semN1");
	sem_unlink("/semN2");
	semH1 = sem_open("/semH1",O_CREAT|O_EXCL,0644,1) ;
	semH2 = sem_open("/semH2",O_CREAT|O_EXCL,0644,0) ;
	semN1 = sem_open("/semN1",O_CREAT|O_EXCL,0644,0) ;
	semN2 = sem_open("/semN2",O_CREAT|O_EXCL,0644,0) ;
	if (fork()) {
		// padre P
		if (fork()) {
			// padre P
			close(pipe1[0]);
			close(pipe1[1]);
			printf("P pid=%d\n",getpid());
			while(wait(NULL) != -1);
			printf("\n\E[0;31m    Fin del proceso Padre!\n \n");
		} else {
			// hijo! H2
			if ( fork() ) {
				// hijo! H2
				printf("H2 pid=%d\n",getpid());
				close(pipe1[0]);
				char linea[255];
				while(strncmp(linea,"chau",4) != 0) {
					sem_wait(semH2);
					printf("\E[0;32m			H2 >> ");
					gets(linea);
					write(pipe1[1],linea,strlen(linea));
					sem_post(semN1);
				}
				close(pipe1[1]);
				printf("\E[0;32m			H2 fin!\n");
			} else {
				// nieto! N2
				printf("N2 pid=%d\n",getpid());
				close(pipe1[1]);
				char linea[255];
				int n;
				while(strncmp(linea,"chau",4) != 0) {
					sem_wait(semN2);
					n = read(pipe1[0],linea,255);
					linea[n]='\0';
					printf("\E[0;32m			N2 << %s\n",linea);
					sem_post(semH2);
				}
				close(pipe1[0]);
				printf("			N2 fin!\n");
			}
		}
	} else {
		// hijo! H1
		if ( fork() ) {
			// hijo! H1
			printf("H1 pid=%d\n",getpid());
			close(pipe1[0]);
			char linea[255];
			sleep(1);
			while(strncmp(linea,"chau",4) != 0) {
				sem_wait(semH1);
				printf("\E[0;33m H1 >> ");
				gets(linea);
				write(pipe1[1],linea,strlen(linea));
				sem_post(semN2);
			}
			close(pipe1[1]);
			printf("\E[0;33m H1 fin!\n");
		} else {
			// nieto! N1
			printf("N1 pid=%d\n",getpid());
			close(pipe1[1]);
			char linea[255];
			int n;
			while(strncmp(linea,"chau",4) != 0) {
				sem_wait(semN1);
				n = read(pipe1[0],linea,255);
				linea[n]='\0';
				printf("\E[0;33m N1 << %s\n",linea);
				sem_post(semH1);
			}
			close(pipe1[0]);
			printf(" N1 fin!\n");
		}
	}
	return 0;
}
