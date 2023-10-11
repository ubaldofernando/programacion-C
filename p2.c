
/* Implementar un proceso al cual se le indique por linea de comando la cantidad de procesos a
crear, todos los procesos a crear serán hermanos; cada uno de ellos retornará un valor entero
distinto al proceso padre y emitirá un mensaje en pantalla antes de finalizar. El proceso padre
reportará por pantalla el retorno recibido de cada proceso hijo y no permitirá que existan
procesos huérfanos o zombies. */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv) {
	int i;
	if ( argc == 2 ) {
		int n = atoi(argv[1]),estado; 
		pid_t pid,wpid;
		if ( n > 0 ) {
			printf("valor de n: %d\n",n); 
			for(i=0;i<n;i++) {
				pid = fork();
				if ( pid == 0 ) {
					printf("proceso hijo %d , hijo de %d  con i= %d\n",getpid(),getppid(),i);
					sleep(1);
					exit(i);
				}
			}
			while((wpid = wait(&estado)) != -1);
		} else {
			printf("%d es un valor incorrecto, debe ser mayor que cero\n",n);
			}
		printf("fin de proceso padre, con i= %d\n",i);
	} else {
		printf("¡Error!, forma de uso:\n./fork5 <cantidad de procesos>\n");
	}
	exit(i);
}
