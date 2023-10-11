#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>

// definicion de los comandos utilizados por el shell
#define SH_EXIT    1
#define SH_CMD   2
#define SH_CD     3
#define SH_PP     4

#define SH_ERROR 98
#define SH_NONE  99

// cual será el prompt de este shell
#define SH_PROMPT  "#>>"
//#define SH_INPUT      '('  // '<'
//#define SH_OUTPUT   ')'  // '>'

// estructura global utilizada por el shell
struct {
   pid_t pid;                      // id de proceso shell
   pid_t pgid;                    // id de grupo de proceso shell
   int interactivo;             // el proceso que pretende ejecutar el usuario es interactivo?
   int input;                     // descriptor de archivo de stdin, entrada estandard
//   int cuentosalida;         // contador usado para generar archivo salidaxxx.out cuando el usuario
                                       // ejecuta proceso no interactivo y no redirije la salida a un archivo
   char *comando;          // comando ingresado por teclado por el usuario (a ser parseado)
   char *tmpcomando;    
   char *error;                // errores controlados por el shell, ej. error en parsing de comando
   char *arg[64];            // comando dividido en tokens (separado por uno o mas espacios), todo
                                      // comando tiene un maximo de 64 argumentos
   int narg;                     // numero actual de argumentos que tiene el comando a ejecutar
   int op;                        // codigo de operacion a ejecutar por el usuario, lo asigna la rutina de
                                      // parsing, se asigna con alguno de las definiciones SH_*
   int salir;                     // flag global para indicar la salida del shell
//   char *entrada;          // nombre del archivo en donde se redirige la entrada del proceso a ejecutar
//   char *salida;             // nombre del archivo en donde se redirige la salida del proceso a ejecutar
   struct termios tmodes; // estructura que contiene todos los atributos actuales de la terminal del shell
} sh;

// prototipo de las funciones principales (nivel 0)
void ingreso();            // muestra el prompt del shell, realiza ingreso de comando, parsing comando 
void inicio();                // inicializa la estructura global del shell
void libero();               // libero todo tipo de memoria dinamica asignada previamente para ejecutar comando
void proceso();           // procesa el comando ingresado por el usuario

// prototipo de las funciones no principales (nivel 1)
void controlHijo(int p);  // funcion que manipula la señal SIGCHLD
void ejecuto();               // llamada por proceso(), crea proceso hijo: fork(), ejecuta proceso hijo, 
                                      // pone en espera al padre
void ingresoLinea();      // llamada x ingreso(), realiza el ingreso de comando por teclado
void parseoLinea();       // llamada x ingreso(), divide a comando por tokens (uno o mas espacios)
int printError();             // muestra sh.error
//void printSh();              // muestra sh
void prompt();              // llamada x ingreso(), muestra prompt del shell

// prototipo de las funciones no principales (nivel 2)
void ejecutoProceso();    // llamada  x ejecuto(), redirije entrada/salida de proceso en foreground/background

//=======================================================================================
//    IMPLEMENTO FUNCIONES (nivel 0)  
//=======================================================================================

int main(void) {
   inicio();
   while(!sh.salir) {
      ingreso();
      //printSh();
      proceso();
      //printSh();
      libero();
   }
   return 0;
}

// realizo el ingreso, lo cual implica mostrar el prompt,ingresar comando,parsear comando
void ingreso() {
   prompt();
   ingresoLinea();
   parseoLinea();
}

// inicializa la estructura sh, ejecuta shell en foreground, establece señales
void inicio() {
   sh.pid = getpid();       // id de proceso actual
/*ver si va*/sh.input = STDIN_FILENO; // descriptor de archivo de stdin
   // isatty() devuelve true si STDIN_FILENO es el descriptor asociado con la terminal, 
   // por lo tanto, se esta ejecutando interactivamente
   sh.interactivo = isatty(sh.input); 
   sh.comando=NULL;
   sh.tmpcomando=NULL;
 //  sh.entrada=NULL;
 //  sh.salida=NULL;
   sh.error=NULL;
   // inicializo vector de argumentos
   memset(sh.arg,0,64*sizeof(char *));
   sh.narg=0;
   sh.op=0;
   sh.salir=0;
//   sh.cuentosalida=0;
//   sh.entrada=NULL;
//   sh.salida=NULL;
   if (sh.interactivo) { // el shell es interactivo?
      /* La señal SIGTTIN es enviada a un proceso o grupo de procesos cuando se pretende
         leer de stdin en un proceso que se esta ejecutando en background, la accion por
         defecto es detener el proceso */
      /* la funcion tcgetpgrp() nos dice cual es el grupo de procesos que controla la terminal,
         el cual debe coincidir con el grupo de proceso del shell, caso contrario, se enviara
         la señal SIGTTIN al grupo de proceso del shell, seguramente provocando la finalizacion
         de los procesos. El programa debe asegurarse que el shell esta en foreground, que controla
         la terminal, incluso aunque sea un proceso hijo de otro shell */
      /* la funcion kill() permite el envio de señales a procesos, pero tambien a grupos de
         proceso, cuando el valor a enviar es menor que -1 se asume que la señal se envia al
         grupo de procesos y no a un proceso en particular */
/*ver esto*/while (tcgetpgrp(sh.input) != (sh.pgid = getpgrp())) 
         kill(- sh.pgid, SIGTTIN);
      /**
       * Este shell ignorará las siguientes señales:
       */
      signal(SIGINT, SIG_IGN);    // interrupcion de programa (presionando Ctrl+C)
      signal(SIGQUIT, SIG_IGN);   // idem anterior, pero ademas hace un core dump,Ctrl+barra invertida
      signal(SIGTSTP, SIG_IGN);   // el usuario pretende suspender este proceso SUSP, Ctrl+Z,
                                                // similar a SIGSTOP aunque ésta no es interactiva y no puede
                                                // ser ignorada, manipulada o bloqueada
      signal(SIGTTIN, SIG_IGN);     // explicada mas arriba
      signal(SIGTTOU, SIG_IGN);   // idem SIGTTIN pero con la salida
      signal(SIGCHLD, &controlHijo); // señal enviada al proceso padre cuando cualquiera de sus 
                                                      // hijos termina o es detenido
      // Ahora el shell es el lider de su nuevo grupo de procesos
      setpgid(sh.pid,sh.pid);
      // obtengo el id del grupo de proceso al cual pertenece el shell
      sh.pgid = getpgrp();
      // si no coincide con el proceso actual, entonces ha fallado este shell en convertirse en lider
      // de este grupo de proceso
      if (sh.pid != sh.pgid) {
         fprintf(stderr,"Error, este shell no puede promoverse como lider de su grupo\n");
         exit(EXIT_FAILURE);
      }
      // el actual grupo de proceso debe ser quien controla la terminal, esta funcion puede fallar
      // en un SO que no soporte control de jobs, en tal caso, se obtienen los atributos actuales
      // de la terminal
      if (tcsetpgrp(sh.input, sh.pgid) == -1)
         tcgetattr(sh.input, &sh.tmodes);
   } else {
      fprintf(stderr,"Error, no es posible hacer a este shell interactivo\n");
      exit(EXIT_FAILURE);
   }
}

// realizo un free() de todo lo previamente asignado con malloc(), tratando de evitar
// un "memory leak" por mal uso de memoria dinámica
void libero() {
   int i;
   if ( sh.comando != NULL ) { free(sh.comando);sh.comando=NULL; }
   for(i=0;i<64;i++) { 
      sh.arg[i]=NULL;
   }
 //  sh.entrada = NULL;
 //  sh.salida = NULL;
   // libero buffer temporario usado por strtok() en funcion de parsing
   if ( sh.tmpcomando != NULL ) { free(sh.tmpcomando);sh.tmpcomando=NULL; }
   if ( sh.error != NULL ) { free(sh.error);sh.error=NULL; }
}

// proceso el o los comandos definidos para este shell
// en el caso de SH_CMD mando a ejecutar el proceso, puesto que se considera que no es
// ningun comando shell, se asume que es un proceso ejecutable del usuario
// luego de la ejecucion, muestro el error resultante -si es que hubo algo-
void proceso() {
   if ( sh.comando == NULL ) return;
//   printf("ejecuto [%s]\n",sh.comando);
   char bufout[32];
   switch(sh.op) {
      case SH_EXIT:
         sh.salir=1;
         break;
      case SH_CD:
         if (sh.arg[1] == NULL) {
            char *curdir = getcwd(NULL,0);
            if ( curdir != NULL ) printf("%s\n",curdir);
            else {
               snprintf(sh.error,128,"proceso(): Error intentando obtener el directorio actual"); 
               sh.op=SH_ERROR;
            }
         } else {
            if (chdir(sh.arg[1]) == -1) {
               snprintf(sh.error,128,"proceso(): [%s] directorio invalido",sh.arg[1]); 
               sh.op=SH_ERROR;
            }
         }
         break;
//      case SH_PP:
//         printf("proceso():aca debo implementar el comando pp!\n");
//         break;
      case SH_CMD:
         if (!sh.interactivo /* && sh.salida == NULL */ ) {
//            snprintf(bufout,32,"salida_%d.out",++sh.cuentosalida);
//            sh.salida = bufout;
            printf("Proceso background [%s]\n" /*salida en [%s]\n"*/,sh.comando/*,sh.salida*/);
         }
         ejecuto();
         break;
      case SH_NONE:
         break;
   }
   printError();
}

//=======================================================================================
//    IMPLEMENTO FUNCIONES (nivel 1)  
//=======================================================================================

/**
 * Funcion asociada a señal de proceso hijo para su control
 * Recupera e imprime informacion acerca del proceso hijo que esta notificando al padre
 * El proceso hijo ha finalizado, sin embargo, el SO no ha eliminado su PCB de la lista
 * procesos. Si ello se hubiera hecho, este codigo no seria posible. 
 */
void controlHijo(int p) {
   pid_t pid;
   int terminationStatus,sig,ret;
   // #define WAIT_ANY no encontrado en cygwin, valor equivalente: -1
   // recupero informacion acerca del proceso hijo que envio la señal al padre
   pid = waitpid(-1, &terminationStatus, WUNTRACED | WNOHANG);
   if (pid > 0) {                                   // hay informacion acerca del proceso hijo?
      
      //pid_t termpid = tcgetpgrp(sh.input); // quien controla la terminal
      //if ( sh.pgid != termpid )  tcsetpgrp(sh.input, sh.pgid);  // el shell nuevamente controla la terminal
/* ? */      usleep(20000);
//creo que esto puedo obviarlo:      
      // uso de macros definidas en <sys/wait.h>
      if (WIFEXITED(terminationStatus)) {           // el proceso finalizo normalmente?
         ret=WEXITSTATUS(terminationStatus);        // devuelve el exit() status del proceso
         printf("Proceso [%d] finalizado Ok y retorno [%d] al SO!\n", pid,ret); 
      } else if (WIFSIGNALED(terminationStatus)) {  // el proceso fue matado por el operador u otro proceso?
                                                    // el proceso hijo recibio una señal que no fue manejada por El
         sig=WTERMSIG(terminationStatus);           // ¿cual señal provoco la eliminacion del proceso hijo?
         printf("Proceso [%d] eliminado! por señal [%d]\n", pid, sig);
      } else if (WIFSTOPPED(terminationStatus)) {   // el proceso ha sido detenido?
         sig=WSTOPSIG(terminationStatus);           // ¿cual señal provoco la detencion del proceso hijo?
         printf("Proceso [%d] suspendido (¿esperando input?) por señal [%d]\n", pid, sig);
      }
      if (WCOREDUMP(terminationStatus)) {
         printf("Proceso [%d] ha generado un core dump\n", pid);
      }
      
      //if ( sh.pgid != termpid )  tcsetpgrp(sh.input, termpid);  // el shell nuevamente controla la terminal
   }
}

/**
 * Realiza un fork()+exec (en funcion ejecutoProceso()) para cambiar
 * la imagen del proceso hijo por otra indicada por el usuario
 */
void ejecuto() {
   pid_t pid;
   pid = fork();
   switch (pid) {
      case -1:  // error en fork(), padre
         strcpy(sh.error,"ejecuto():Error en fork()");
         sh.op = SH_ERROR;
         break;
      case 0:   // hijo
         /**
          *  el proceso hijo hereda los manipuladores de señales del padre
          *  se ponen las señales a su valor por defecto, excepto para la
          *  señal de finalizacion del proceso hijo cuando éste se ejecuta
          *  en modo batch (background), de esta forma, el proceso padre (el shell)
          *  sera notificado de la finalizacion del hijo
          */
//         signal(SIGINT, SIG_DFL);
//         signal(SIGQUIT, SIG_DFL);
//         signal(SIGTSTP, SIG_DFL);
         if (!sh.interactivo) signal(SIGCHLD, &controlHijo);
         else                      signal(SIGCHLD, SIG_DFL);
         signal(SIGTTIN, SIG_DFL);
         //usleep(20000);             // problema de sincronizacion con procesos pequeños
         setpgrp();                   // se indica al proceso hijo como nuevo lider de este grupo de procesos
         pid_t chpid = getpid();      // en este caso devuelve el id del proceso hijo
         if (sh.interactivo) {        // es una copia de la variable del padre
            // si es un proceso interactivo (en foreground) este proceso debe controlar la terminal
            tcsetpgrp(sh.input,chpid); 
         } else {
            // informo al usuario acerca de la ejecucion en background
            printf("Proceso [%d] ejecutado en background\n", (int) chpid);
         }
         ejecutoProceso();
         if (printError()) exit(EXIT_FAILURE);
         else                 exit(EXIT_SUCCESS);
         break;
      default:  // padre
         setpgid(pid, pid);  // el proceso hijo es el lider de su grupo
         //usleep(10000);
         //usleep(10000);
         // to avoid race conditions
         if (sh.interactivo) {
            tcsetpgrp(sh.input,pid);                                 // el nuevo proceso controla la terminal
            // wait for the child job
            int terminationStatus;
//printf("padre esperando hijo!\n");
            waitpid(pid, &terminationStatus,WUNTRACED);
            //while(waitpid(pid, &terminationStatus,WUNTRACED | WNOHANG) == 0);
//printf("fin padre esperando hijo!\n");
            tcsetpgrp(sh.input, sh.pgid); // el shell nuevamente controla la terminal
         }
         break;
   }
}

/*
Esta funcion realiza un ingreso por teclado basado en getc(stdin), considera que los caracteres
10 o 13 leidos por teclado son indicadores de que ha terminado el ingreso.
El problema es que no se saben cuántos serán los caracteres a ingresar por parte del usuario,
por lo tanto, utiliza memoria dinámica: realiza inicialmente un malloc() de 128 bytes y en dicha
area de memoria solicitada al SO, almacena los caracteres leidos de stdin. Cuando el buffer se
agota, solicita otros 128 bytes adicionales; asi sucesivamente, hasta que termina el ingreso de
caracteres. Por último hace un fflush() del teclado para evitar que quede algun caracter dentro
del buffer de teclado.
*/
void ingresoLinea() {
// se toma el ingreso por stdin de un string hasta presionar intro o bien llegar al maximo de MAXSTRING digitos
   char c;
   const int nbytes=128;
   char *buf = (char *) malloc(nbytes);
   char *bufant = buf;
   int nbufsize=nbytes;
   int nbuf = 0;
   int salir = (buf == NULL);
   fflush(stdin);
   for(;!salir && (c = (char) getc(stdin)) != 10 && c != 13;buf[nbuf++] = c) {
      if ( (nbuf % nbytes) == 0 ) {
         nbufsize+=nbytes;
         bufant=buf;
         buf = (char *) realloc(buf,nbufsize);
         salir=(buf == NULL);
      }
   }
   buf[nbuf] = '\0';
   fflush(stdin);
   if ( salir ) {
      free(bufant);
      sh.comando=NULL;
   } else sh.comando = buf;
}

/*
Esta es una funcion de validacion del comando ingresado por el usuario
Recordar que esto es en funcion del tipo de comandos que acepta este shell, indicado en el texto
del tp correspondiente
Se agregan validaciones obvias como por ejemplo, el comando no podria tener mas de un simbolo
de redireccion de entrada o salida o de ejecucion en background
Si algun error de parsing sucediera, se carga el error correspondiente y se asume SH_ERROR o SH_NONE,
segun corresponda
Si esta todo ok, se divide el comando ingresado en tokens (el comando se separa por uno o mas espacios)
estos tokens se cargan en el arreglo args[] y nargs guarda la cantidad de tokens encontrados, teniendo
en cuenta que no puede superarse la cantidad de 64 tokens
*/
void parseoLinea() {
   sh.op = SH_NONE;
   if ( sh.comando == NULL || strlen(sh.comando) == 0 || strcmp(sh.comando,"") == 0 ) {
      return;
   }
   int i=0;
   char *buffer = (char *) malloc(strlen(sh.comando)+1);
   if (buffer == NULL) { 
      fprintf(stderr,"parseoLinea():Error de asignacion de memoria\n");
      exit(EXIT_FAILURE); 
   }
   sh.tmpcomando = buffer;
   strcpy(buffer,sh.comando);
   buffer = strtok(sh.comando, " ");
   while (i < 64 && buffer != NULL) {
      sh.arg[i] = buffer;
      buffer = strtok(NULL, " ");
      i++;
   }
   sh.narg=i;
   sh.interactivo=1;

   // busco caracteres de redireccion
   int w,nminus=0,nmax=0,nbg=0;
//   int pminus=-1,pmax=-1,pbg=-1;
   for(w=0;w<sh.narg;w++) {
//      if ( *sh.arg[w] == SH_INPUT ) { 
//         nminus++;
//         pminus=w;
//         sh.entrada = sh.arg[w+1];
//      }
//      if ( *sh.arg[w] == SH_OUTPUT ) { 
//         nmax++;
//         pmax=w;
//         sh.salida = sh.arg[w+1];
//      }
      if ( *sh.arg[w] == '&' ) { 
//         nbg++;
//         pbg=w;
         sh.interactivo=0;
      }
   }
   // valido comando
   sh.error = (char *) malloc(128);
   if (sh.error == NULL) { 
      fprintf(stderr,"parseoLinea():Error de asignacion de memoria(2)\n");
      exit(EXIT_FAILURE);
   }
   if ( sh.narg == 0 ) {
      strcpy(sh.error,"parseoLinea():Error, no hay comando  a ejecutar");
      sh.op = SH_ERROR;
      return;
   }
/*   int poserror=( pminus == 0 || pmax == 0 || pbg == 0 );
   if ( poserror || nminus > 1 || nmax > 1 || nbg > 1 ) {
      strcpy(sh.error,"parseoLinea():Error de Sintaxis en comando");
      sh.op = SH_ERROR;
      return;
   }   */
   // determino comando a ejecutar
   if ( strcmp(sh.arg[0],"salir") == 0 ) sh.op = SH_EXIT;
   else if ( strcmp(sh.arg[0],"cd") == 0 ) sh.op = SH_CD;
   else if ( strcmp(sh.arg[0],"pp") == 0 ) sh.op = SH_PP;
   else sh.op = SH_CMD;
}

/**
 * Imprime -si es necesario- el error del último comando enviado al shell
 * Devuelve true en caso de existir error, caso contrario, devuelve false
 */
int printError() {
   if ( sh.op == SH_ERROR) printf("Error [%s] ejecutando [%s]\n",sh.error,sh.comando);
   return ( sh.op == SH_ERROR);
}

/**
 * Para proposito de debug/trace
 * Imprime datos actuales del shell
 */
void printSh() {
   printf("\n==================================\n");
   int w=0;
   printf("cmd=[%s] int=%d input=%d nargs=%d\n",sh.comando,sh.interactivo,sh.input,sh.narg);
   for(;w<sh.narg+3;w++) printf("arg[%d]=[%s]\n",w,sh.arg[w]);
//   printf("entrada=[%s] salida=[%s] error=[%s]\n",sh.entrada,sh.salida,sh.error);
   printf("\n==================================\n");
}

// muestro prompt del shell
void prompt() {
   printf("\n");
   printf(SH_PROMPT);
}

//=======================================================================================
//    IMPLEMENTO FUNCIONES (nivel 2)  
//=======================================================================================

/**
 * Esta funcion se ejecuta dentro del contexto de un proceso hijo (que recien ha sido forkeado), antes
 * de reeemplazar su codigo (actualmente es una copia de su padre) por el codigo del proceso que
 * el operador pretenda ejecutar, se debe tener en cuenta como se redirigira la entrada y la salida
 * de este nuevo proceso.
 * Si es un proceso en background y no se ha redirigido su salida, para que la misma no salga en la consola
 * y confunda al operador, el shell la redirige automaticamente a un archivo llamado SALIDA_<nro.salida>.out
 * que fue previamente informado al usuario (ver funcion proceso())
 * Si la entrada/salida ya habia sido redirigida por el operador, el programa abre los archivos de entrada
 * y salida indicados por el operador y los conecta con el STDIN/STDOUT del nuevo proceso
 */
void ejecutoProceso() {
/*    int infile,outfile;
   if (sh.entrada != NULL) {
      infile = open(sh.entrada, O_RDONLY, 0777);
      if ( infile != -1 ) {
         dup2(infile, STDIN_FILENO);
         close(infile);
      } else printf("ejecutoProceso():Error redirigiendo entrada\n");
   }
   if (sh.salida != NULL) {
      outfile = open(sh.salida, O_WRONLY | O_CREAT | O_TRUNC, 0777); 
      if ( outfile != -1) {
         dup2(outfile, STDOUT_FILENO);
         close(outfile);
      } else printf("ejecutoProceso():Error redirigiendo salida[%s]\n",sh.salida);
   }
*/   // limpio argumentos de redireccion
   // sh.arg[] es un buffer read-only, paso su info importante a tmp[] para usarlo con execvp()
   char *tmp[sh.narg+1];
   int i,w=0;
   for(i=0;i<sh.narg;i++) {
      if (/* *sh.arg[i] == SH_INPUT || *sh.arg[i] == SH_OUTPUT || */ *sh.arg[i] == '&' ) break;
      tmp[w] = (char *) malloc(strlen(sh.arg[i])+1);
      strcpy(tmp[w],sh.arg[i]);
      w++;
   }
   for(;w<sh.narg+1;w++) tmp[w]=NULL;
   //for(w=0;w<sh.narg+1;w++) printf("tmp[%d]=[%s]\n",w,tmp[w]);
   
//printf("ejecutoProceso():ejecuto!\n");
   if (execvp(*tmp, tmp) == -1) {
      strcpy(sh.error,"ejecutoProceso(): Error en execvp()"); 
      sh.op=SH_ERROR;
   }
}

