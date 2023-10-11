/** p8.c                          Ubaldo, Fernando M */

/* Implemente un proceso que quede bloqueado a la espera de recibir la 
 * señal SIGUSR1 (antes de ello, que indique en pantalla cuál es su
 * process id (PID)) cuando reciba esta señal el proceso deberá imprimir
 * el abecedario en minúsculas y luego volverá a quedar bloqueado; la 
 * próxima vez que reciba SIGUSR1 imprimirá el abecedario en mayúsculas
 * y así sucesivamente hasta que reciba SIGUSR2 en ese caso, el programa
 * terminará su ejecución.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

void sig_user1(int);
void sig_user2(int);

sigset_t set;
int cont=0;
char letramin='a';
char letramay='A';

int main(int argc, char **argv) {
	printf("\n   Inicio proceso %d\n \n",getpid());
	signal(SIGUSR1,sig_user1);
	signal(SIGUSR2,sig_user2);
	sigfillset(&set); 
	sigdelset(&set,SIGUSR1);  
	sigdelset(&set,SIGUSR2);  
	sigprocmask(SIG_BLOCK,&set,NULL);
	while(1);
	return 0;
}

void sig_user1(int signo) {	 //signo=nro de señal
	if ((cont%2) == 0 ) {   
		while(letramin <='z') {
			printf(" %c",letramin++);
		}
		printf("\n");   //para vaciar el buffer (se imprime todo el abecedario)
		letramin='a';
    } else {
		while(letramay <='Z') {
			printf(" %c",letramay++);
		}
		printf("\n"); // para vaciar el buffer (se imprime todo el abecedario)
		letramay='A';
	}
	cont++;
}

void sig_user2(int signo) {
	printf("\n Se recibio la señal SIGUSR2, que indica la finalizacion del proceso\n\n");
	exit(0);
}
