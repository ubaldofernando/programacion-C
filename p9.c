/** p9.c                         Ubaldo, Fernando M */

/* Realizar un chat entre procesos emparentados usando pipes. Crear una
 * estructura de procesos emparentados de cinco procesos, un proceso
 * padre creará dos procesos hijos y cada proceso hijo creará un proceso
 * nieto. La creación de procesos se realiza con la llamada al sistema 
 * fork. El chat se realizará entre los procesos hijos que serán los 
 * escritores y cada proceso nieto será el lector de su correspondiente
 * escritor. Se decide usar dos pipes como recursos de comunicación
 * entre procesos para realizar una programación más simple y enfocar el
 * problema al concepto. Se recomienda que el alumno realice el mismo
 * ejercicio utilizando solo un pipe.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

int main(int argc, char **argv) {
	int pipe1[2];  
	int pipe2[2];  
	pipe(pipe1);
	pipe(pipe2);
	if (fork()) {
		// padre P
		if (fork()) {
			// padre P
			close(pipe1[0]);
			close(pipe1[1]);
			close(pipe2[0]);
			close(pipe2[1]);
			printf("\n  		 Chat\n");
			while(wait(NULL) != -1);
			sleep(1);
			printf("\n\E[0;31m Fin del proceso Padre\n \n");
		} else {
			// hijo! H1
			if (fork()) {
				// hijo! H1
				close(pipe2[0]);
				close(pipe1[1]);
				close(pipe1[0]);
				char linea[255];
				while(strncmp(linea,"chau",4) != 0) {
					printf("\n\E[0;32m			H1 >> ");
					gets(linea);
					write(pipe2[1],linea,strlen(linea));
				}
				close(pipe2[1]);
				printf("\n\E[0;32m			H1 fin!\n");
			} else {
				// nieto! N1
				close(pipe2[0]);
				close(pipe1[1]);
				close(pipe2[1]);
				char linea[255];
				int n;
				while(strncmp(linea,"chau",4) != 0) {
					n = read(pipe1[0],linea,255);
					linea[n]='\0';
					printf("\n\E[0;32m			N1 << %s\n",linea);
				}
				close(pipe1[0]);
				printf("\n			N1 fin!\n");
			}
		}
	} else {
		// hijo! H2
		if (fork()) {
			// hijo! H2
			close(pipe2[0]);
			close(pipe2[1]);
			close(pipe1[0]);
			char linea[255];
			while(strncmp(linea,"chau",4) != 0) {
				printf("\n\E[0;33m H2 >> ");
				gets(linea);
				write(pipe1[1],linea,strlen(linea));
			}
			close(pipe1[1]);
			printf("\n\E[0;33m H2 fin!\n");
		} else {
			// nieto! N2
			close(pipe1[0]);
			close(pipe2[1]);
			close(pipe1[1]);
			char linea[255];
			int n;
			while(strncmp(linea,"chau",4) != 0) {
				n = read(pipe2[0],linea,255);
				linea[n]='\0';
				printf("\n\E[0;33m N2 << %s\n",linea);
			}
			close(pipe2[0]);
			printf("\n 		N2 fin!\n");
		}
	}
	return 0;
}

