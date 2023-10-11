/** p7.c                            Ubaldo, Fernando M */

/* Problema: Una empresa tiene 5 sucursales de las cuales registra las
 * ventas de un mes, ejercicio propuesto en práctica titulada “TP III – 
 * Procesos Livianos – Threads”; implemente la versión más compleja para 
 * la solución al problema de la Empresa con 5 sucursales haciendo uso 
 * de variables de condición. Solo realice el programa C, no es 
 * necesario que adjunte con el código las respuestas a las preguntas
 * que se proponen en los puntos 5 y 6 de lapráctica “TP III – Procesos
 * Livianos – Threads”.
 */

#include <pthread.h>
#include <unistd.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCURSAL 5

double venta[SUCURSAL][31];
double total=0.0;
double totalxsuc[SUCURSAL];

pthread_mutex_t m1[SUCURSAL];
pthread_cond_t cargado[SUCURSAL];

int flagcargada[SUCURSAL];

void *sumoVentas(void *);
void *cargadeventas(void *);

int main() {
	pthread_t hilos[SUCURSAL];
	pthread_t cargohilo;
    int h, s;
    memset(&flagcargada,0,sizeof(int)*SUCURSAL);
    memset(&venta,0,sizeof(double)*SUCURSAL*31);
    memset(&totalxsuc,0,sizeof(double)*SUCURSAL);
    for(h=0; h<SUCURSAL; h++) {
		pthread_mutex_init(&m1[h],NULL);
        pthread_cond_init(&cargado[h],NULL);
    }
    printf("\n Creando hilo para carga de ventas\n");
    pthread_create(&cargohilo,NULL,cargadeventas,NULL);
    sleep(1);
    for(h=0; h<SUCURSAL; h++) {
		s=h+1;
        printf(" Creando hilo %d\n",s);
        pthread_create(&hilos[h],NULL,sumoVentas,(void *) h);
    } 
	sleep(1);
	for(h=0; h<SUCURSAL; h++){
        pthread_join(hilos[h],NULL); 
    }
    pthread_join(cargohilo,NULL);
	for(h=0; h<SUCURSAL; h++) total+=totalxsuc[h];
	printf("\n Calculando Total General\n \n");
	sleep(1);
    printf("\n     Total Gral $%9.2f\n \n",total);
    pthread_exit(NULL);
}

void *sumoVentas(void *idsuc) {
	int sucursal, s;
	sucursal=(int) idsuc;
	s=sucursal+1;
	printf("\n Hilo %d esperando carga de sucursal %d \n",s,s);
	pthread_mutex_lock(&m1[sucursal]);
	if (!flagcargada[sucursal]) {
		printf(" Hilo %d esperando var de condicion 'cargado' para sucursal %d \n",s,s);
		pthread_cond_wait(&cargado[sucursal],&m1[sucursal]);
	}
	int dia;
	for(dia=0;dia<31;dia++) {
		totalxsuc[sucursal]+=venta[sucursal][dia];
	}
	sleep(1);
	printf("\n Fin hilo %d\n  Suma de sucursal %d  $%9.2f\n",s,s,totalxsuc[sucursal]);
	pthread_mutex_unlock(&m1[sucursal]);
	pthread_exit(NULL);
}

void *cargadeventas(void *p) {
	int suc,dia;
	printf("\n Hilo carga de ventas\n \n");
	for(suc=0;suc<SUCURSAL;suc++) {
		pthread_mutex_lock(&m1[suc]);
		for(dia=0;dia<31;dia++) {
			venta[suc][dia] = ((double) random())/10000000.0;
		}
		flagcargada[suc]=1;
		pthread_cond_signal(&cargado[suc]);
		pthread_mutex_unlock(&m1[suc]);
		sleep(3);
	}
	sleep(1);
	printf("\n Fin hilo cargadeventas\n");
	pthread_exit(NULL);
}
