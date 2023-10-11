/** fshell.c                         Ubaldo, Fernando M */

/* Implementación del programa Shell (en su versión más completa, que 
 * soporta procesos foreground y background; que haga uso de la señal
 * SIGCHLD para verificar la finalización de procesos en background)
 * propuesto en práctica titulada “TP II – Procesos Pesados”.
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>

#define FORE 1
#define BACK 0
#define NBYTES 128
#define OPSALIR 1
#define OPCOMANDO 2
#define OPCD 3
#define OPERROR 4
#define OPNOCOMANDO 5
#define ENTRADA '<'
#define SALIDA '>'

struct {
	pid_t pid;
	pid_t pgid;
	int interactivo;
	int input;
	char *comando;
	char *error;
	char *arg[64];
	int narg;
	int op;
	int salir;
	int csalida;
	char *entrada;
	char *salida;
	struct termios atr; // contiene todos los atributos del shell
} sh;

void inicio();
void ingreso();
void proceso();
void libero();
void miprompt();
void ingresoLinea();
void parseoLinea();
void ejecutoCD();
void ejecutoComando();
int imprimError();
void procesoHijo();
void controlHijo();

int main(void) {
	inicio();
	while(sh.salir != 1) {
		ingreso();
		proceso();
		libero();
	}
	return 0;
}
   
void inicio() {
	sh.pid=getpid();
    sh.input =0;
    sh.comando=NULL;
    sh.entrada=NULL;
    sh.salida=NULL;
    sh.error=NULL;
    sh.csalida=0;
    sh.interactivo=FORE;
    memset(sh.arg,0,64*sizeof(char *));
    sh.narg=0;
    sh.op=0;
    sh.salir=0;
	sh.entrada=NULL;
    sh.salida=NULL;
	if (sh.interactivo) {
		sh.pgid = getpgrp();     
		signal(SIGTTOU, SIG_IGN);  //SIGTTOUT-> no permite que un trabajo de fondo escriba
								   //           en el terminal o establesca sus modos.
		signal(SIGCHLD, &controlHijo);                                                  
		setpgid(sh.pid,sh.pid);
		if (tcsetpgrp(sh.input, sh.pgid) == -1)
			tcgetattr(sh.input, &sh.atr);
	} else {
		printf("Error, no es posible hacer a este shell interactivo\n");
		exit(1);
	}
}

void ingreso() {
	miprompt();
	ingresoLinea();
	parseoLinea();
}
	
void proceso() {
	if (sh.comando == NULL) return;
	switch(sh.op) {
		case OPSALIR:
			sh.salir=1;
			break;
		case OPCD:
			ejecutoCD();
			break;
		case OPCOMANDO:
			ejecutoComando();
			break;
		case OPNOCOMANDO:
			break;
	}
	imprimError();
}

void ejecutoCD() {
	if (sh.arg[1] == NULL) {
		char *dir = getcwd(NULL,0);
        if (dir != NULL) { 
			printf("%s\n",dir);      
        } else {
			snprintf(sh.error,128,"Error al obtener el directorio actual"); 
            sh.op=OPERROR;
		}
	} else {
		if (chdir(sh.arg[1]) == -1) {
			snprintf(sh.error,128,"'%s' directorio invalido",sh.arg[1]); 
			sh.op=OPERROR;
		}
	}
}

void ejecutoComando() {
	char buffout[32];
	if (!sh.interactivo && sh.salida == NULL) {
		snprintf(buffout,32,"salida_%d.out",++sh.csalida);
        sh.salida = buffout;
		printf("Proceso background  %s , salida en: %s\n",sh.comando,sh.salida);
    }
	pid_t pid;
	pid_t chpid;
	pid = fork();
	switch (pid) {
		case -1:  
			strcpy(sh.error,"Error en fork()\n");
			sh.op = OPERROR;
			break;
		case 0:
			setpgrp();                   
			chpid = getpid();     
			if (sh.interactivo) { 
				tcsetpgrp(sh.input,chpid); 
			} else {
				printf("Proceso  %d  ejecutado en background\n", (int) chpid);
			}
			procesoHijo();
			if (imprimError()) exit(1);
			else exit(0);
			break;
		default:
			setpgid(pid, pid);
			if (sh.interactivo) {
				tcsetpgrp(sh.input,pid);
				int terminationStatus;
				waitpid(pid, &terminationStatus,WUNTRACED);
				tcsetpgrp(sh.input, sh.pgid);
			}
			break;
	}
}

void libero() {
	int i;
    sh.interactivo=FORE;
    sh.input=0;
    sh.narg=0;
    sh.op=0;
    if (sh.comando != NULL) {
		free(sh.comando);
	    sh.comando=NULL;
    }
    for(i=0;i<64;i++) { 
        sh.arg[i]=NULL;
    }
    sh.entrada = NULL;
    sh.salida = NULL;
    if ( sh.error != NULL ) { 
	    free(sh.error);
	    sh.error=NULL;
	}
}

void miprompt() {
	sleep(1);
	printf("\n ~usuario~ :- ");
}

void ingresoLinea() {
	int nbuffsize = NBYTES;
	char c;
	char *buff = (char *) malloc(NBYTES);
	int nbuff = 0;
	char *buff2=buff;
	int salir = (buff==NULL);
	fflush(stdin);
	for(;salir!=1 && (c = (char) getc(stdin)) != 10 && c != 13;buff[nbuff++] = c) {
		if ((nbuff % NBYTES) == 0) {
			nbuffsize += NBYTES;
			buff2=buff;	
			buff = (char *) realloc(buff,nbuffsize);
			salir=(buff==NULL);
		}
	}
	buff[nbuff]='\0';
	fflush(stdin);
	if (salir) {
		free(buff2);
		sh.comando=NULL;
	} else sh.comando=buff;
}
  
void parseoLinea() {
	int c=0;
	sh.op = OPNOCOMANDO;
	if ( sh.comando == NULL || strlen(sh.comando) == 0 || strcmp(sh.comando,"") == 0 ) {
		return;
	}
	char *buff=(char *) malloc(strlen(sh.comando)+1);
	if (buff == NULL) {
		printf("Error de asignacion de memoria \n");
		exit(1);
	} 
	strcpy(buff,sh.comando);
	buff = strtok(sh.comando," ");
	while (c < 64 && buff != NULL) {
		sh.arg[c] = buff;
		buff = strtok(NULL, " ");
		c++;
	}
	sh.narg=c;
	sh.interactivo=FORE;
	int w;
	for(w=0;w<sh.narg;w++) {
		if (*sh.arg[w] == ENTRADA) { 
			sh.entrada = sh.arg[w+1];
		}
		if (*sh.arg[w] == SALIDA ) { 
			sh.salida = sh.arg[w+1];
		}
		if (*sh.arg[w] == '&') { 
			sh.interactivo=BACK;
		}
	}
	sh.error = (char *) malloc(128);
	if (sh.error == NULL) { 
		printf("Error de asignacion de memoria \n");
		exit(1);
	}
	if (sh.narg == OPNOCOMANDO) {
		strcpy(sh.error,"Error, no hay comando a ejecutar\n");
		sh.op = OPERROR;
		return;
	}
	if (strcmp(sh.arg[0],"salir") == 0) sh.op = OPSALIR;
	else if (strcmp(sh.arg[0],"cd") == 0) sh.op = OPCD;
	else sh.op = OPCOMANDO;
}

int imprimError() {
	if (sh.op == OPERROR) printf("Error %s ejecutando '%s'\n",sh.error,sh.comando);
	return (sh.op == OPERROR);
}

void procesoHijo() {
	int fin,fout;
	if (sh.entrada != NULL) {
		fin = open(sh.entrada, O_RDONLY, 0777);
		if (fin != -1) {
			dup2(fin,STDIN_FILENO);				
			close(fin);
		} else printf("Error redirigiendo entrada '%s'\n",sh.entrada);
	}
	if (sh.salida != NULL) {
		fout = open(sh.salida, O_WRONLY | O_CREAT | O_TRUNC, 0777); 
		if (fout != -1) {
			dup2(fout, STDOUT_FILENO);
			close(fout);
		} else printf("Error redirigiendo salida '%s'\n",sh.salida);
	}
	char *t[sh.narg+1];
	int i,w=0;
	for(i=0;i<sh.narg;i++) {
		if (*sh.arg[i] == ENTRADA || *sh.arg[i] == SALIDA || *sh.arg[i] == '&' ) break;
		t[w] = (char *) malloc(strlen(sh.arg[i])+1);
		strcpy(t[w],sh.arg[i]);
		w++;
	}
	for(;w<sh.narg+1;w++) t[w]=NULL;		
	if (execvp(*t, t) == -1) {
		strcpy(sh.error,"Error en execvp()\n"); 
		sh.op=OPERROR;
	}
}

void controlHijo(int p) {
	pid_t pid;
    int status;
    pid = waitpid(-1, &status, WUNTRACED | WNOHANG);
    if (pid > 0) {                                 
		usleep(200000);      
        printf("Proceso  %d  finalizado correctamente y retorno %d al SO\n", pid,status);       
   }
}

