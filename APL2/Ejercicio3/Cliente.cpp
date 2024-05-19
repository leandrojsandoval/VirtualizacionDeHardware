#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include <string>
#include <semaphore.h>
#include <sys/mman.h>
#include <ctime>
#include <cerrno>
#include <linux/fs.h>
#include <sys/param.h>


#define SEND_FIFO "FIFO1"
#define RECEIVE_FIFO "FIFO2"
#define MC "EJ3"

void ayuda();
void comunicacion_fifos2();
void inicializarSemaforo();
void liberar_recursos(int);

sem_t* semaforo[4];
/*
	0 - Servidor
	1 - Cliente
	2 - Turno Servidor
	3 - Turno Cliente
*/

typedef struct{
	int pidServidor;
	int pidCliente;
}dato;

using namespace std;

int main(int argc, char * argv[])
{
	//Ignora el ctrl+C
	signal(SIGINT,SIG_IGN);
	signal(SIGTERM,liberar_recursos);
	//Se validan los parámetros
	if(argc == 2 && ( (strcmp(argv[1], "-h")==0) || (strcmp(argv[1], "--help")==0) ))
	{
		ayuda();
		return EXIT_SUCCESS;
	}

	if(argc > 1)
	{
		printf("Error en el ingreso de parámetros.\n");
		printf("Ingrese \"-h\" o \"--help\" como único parámetro para obtener la ayuda.\n");
		return EXIT_FAILURE;
	}

	//comunicacion_fifos();
	comunicacion_fifos2();
	unlink(SEND_FIFO);
	unlink(RECEIVE_FIFO);	

	return EXIT_SUCCESS;
}

void ayuda()
{
        char opcion[25];
        int num;

        printf ("\nINGRESE UNA OPCIÓN (1-6): \n");
        printf("1) Integrantes\n2) Sinopsis\n3) Parámetros\n4) Descripción\n5) Ejemplo de funcionamiento\n6) Salir\n");

        do{
                printf("\nOpción elegida: ");
                scanf("%s", opcion);
                num = atoi(opcion);

                while ((num == 0) || (num < 1 || num > 6)){
                        printf ("Error, ingrese un número comprendido entre 1 y 6.\n");
                        printf("Opción elegida: ");
                        scanf("%s", opcion);
                        num = atoi(opcion);
                }

                switch(num){
                        case 1:
                                printf("\n=========================== Encabezado =======================\n");

                                printf("\nNombre del script: cliente.cpp");
                                printf("\nNúmero de ejercicio: 3");
                                printf("\nTrabajo Práctico: 3");
                                printf("\nEntrega: Primera reentrega");
								printf("\n\n==============================================================\n");

                                printf("\n------------------------ Integrantes ------------------------\n\n");

                                printf("Matías Beltramone - 40.306.191\n");
                                printf("Eduardo Couzo Wetzel - 43.584.741\n");
                                printf("Brian Menchaca - 40.476.567\n");
                                printf("Ivana Ruiz - 33.329.371\n");
                                printf("Lucas Villegas - 37.792.844\n");
                                
								printf("\n-------------------------------------------------------------\n\n");
                                break;
                        case 2:
                                printf("\n##SINOPSIS##\nUn supermercado desea poder generar algunas consultas sobre los productos que comercializa utilizando una serie de comandos.\n");
                                break;
                        case 3:
                                printf("\n##PARÁMETROS##\n");
                                printf("El proceso cliente no recibe parámetros.\n");
								printf("El proceso servidor recibe un solo parámetro: el nombre del archivo de productos (archivo de texto plano).\n");
								break;
                        case 4:
                                printf("\n##DESCRIPCIÓN##\n\n");
								printf("Se ponen a correr dos procesos (cliente y servidor) que se comunicarán mediante dos FIFOS.\n");
								printf("El proceso cliente recibe comandos que se enviarán al proceso servidor que consultará un archivo de texto.\n");
								printf("Comandos que puede recibir el proceso cliente:\n");
								printf("STOCK producto_id: Muestra DESCRIPCIÓN y STOCK para un producto dado.\n");
								printf("SIN_STOCK: Muestra ID, DESCRIPCIÓN y COSTO de los productos con STOCK cero.\n");
								printf("REPO cantidad: Muestra el costo total de reponer una cantidad dada para cada producto sin stock.\n");
								printf("LIST: Muestra ID, DESCRIPCIÓN y PRECIO de todos los productos existentes.\n");
								printf("QUIT: Finaliza la ejecución.\n");
                                break;
                        case 5:
                                printf("\n##EJEMPLO DE FUNCIONAMIENTO##\n");
								printf("./servidor productos.txt\n");
								printf("./cliente\n");
								printf("En caso de que no se encuentren los archivos ejecutables, se debe compilar y linkeditar arrojando el siguiente comando por consola:\n");
								printf("make\n");
								printf("Para eliminar los ejecutables ejecutar 'make clean'\n");
                                break;
           }
        }
        while(num != 6);
}

void comunicacion_fifos2(){
	//Creamos dos fifos para enviar y recibir mensajes

	inicializarSemaforo();
	sem_wait(semaforo[1]);

	int idAux = shm_open(MC, O_CREAT | O_RDWR, 0600);
    dato *pidA = (dato*)mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, idAux,0);
    close(idAux);
    pidA->pidCliente = getpid();
    munmap(pidA,sizeof(dato));

	mkfifo(SEND_FIFO, 0666);
	mkfifo(RECEIVE_FIFO, 0666);
	
	while(1)
	{
		sem_wait(semaforo[3]);
        ofstream fifo1(SEND_FIFO);
		char tmp[255] = "";
		int ban = 0;
		fflush(stdout);
		fgets(tmp, sizeof(tmp), stdin);
			
		tmp [strlen (tmp) -1] = 0; // cambia el \n de fgets a \0
			
		for (int i = 0; i < strlen(tmp); i++) {
			tmp[i] = toupper(tmp[i]);
		}
			
		if(strcmp(tmp, "LIST") == 0 || strcmp(tmp, "SIN_STOCK") == 0)
		{
            fifo1 << string(tmp) << ends;
			fifo1.close();
			ban = 1;
		}

		if(tmp[0]=='R' && tmp[1]=='E' &&tmp[2]=='P' && tmp[3]=='O' && tmp[4]==' ')
		{
			char aux[249];
			ban = 1;
    		strncpy(aux, &tmp[5], strlen(tmp)-1 );

			int num = atoi(aux);
			
			if(num > 0){
				fifo1 << string(tmp) << ends;
				fifo1.close();
			}
			else{
				printf("Error, la cantidad debe ser mayor o igual a cero.\n");
				printf("Consulte la ayuda con ./cliente -h ó ./cliente --help\n");
				ban = 0;
			}				
		}
		if (tmp[0]=='S' && tmp[1]=='T' &&tmp[2]=='O' && tmp[3]=='C' && tmp[4]=='K' && tmp[5]==' ')
		{
   			char aux[249];
			ban = 1;
   			strncpy(aux, &tmp[6], strlen(tmp)-1 );
			int num = atoi(aux);
			if(num >0){
				fifo1 << string(tmp) << ends;
				fifo1.close();
			}
			else{
				printf("Error, el ID del producto es mayor o igual a cero.\n");
				printf("Consulte la ayuda con ./cliente -h ó ./cliente --help\n");
				ban = 0;
			}								
		}
		if(strcmp(tmp, "QUIT") == 0)
		{	
			fifo1 << string(tmp) << ends;
			fifo1.close();
			sem_close(semaforo[0]);
			sem_close(semaforo[1]);
			sem_close(semaforo[3]);
			sem_post(semaforo[2]);	//le damos el turno al servidor antes de finalizar nuestro cliente.
			sem_close(semaforo[2]);
			exit(EXIT_SUCCESS);
		}
		if(ban == 1){
			sem_post(semaforo[2]);	// le damos el turno al servidor
			sem_wait(semaforo[3]);	// nos quedamos a la espera de que se libere el turno del cliente.
			ifstream fifo2(RECEIVE_FIFO);
			string buffer;
			ban = 0;
			while(getline(fifo2,buffer))
                cout << buffer << endl;
			fifo2.clear();
			fifo2.close();
			fflush (stdout); 
		}
		else
			printf("Error, accion erronea.\nVuelva a intentarlo.\n");
		sem_post(semaforo[3]);
	}
}

void inicializarSemaforo(){
	semaforo[0] = sem_open("servidorFIFO",O_CREAT,0600,1);
	int valorSemServi = 12;
	sem_getvalue(semaforo[0],&valorSemServi);
	if(valorSemServi == 1){
		printf("El proceso servidor aun no ha sido inicializado\n");
		sem_close(semaforo[0]);
		exit(EXIT_FAILURE);
	}
    semaforo[1] = sem_open("clienteFIFO",O_CREAT,0600,1);
    // Si dicho semaforo vale 0 en ese momento significa que ya hay otra instancia de semaforo ejecutando por lo que cerramos el proceso.
    int valorSemCliente = 85;
    sem_getvalue(semaforo[1],&valorSemCliente);
    if(valorSemCliente == 0){
		printf("Error, ya hay un cliente ejecutando...\n");
		sem_close(semaforo[0]);
		sem_close(semaforo[1]);
        exit(EXIT_FAILURE);
	}
	semaforo[2] = sem_open("turnoServFIFO",O_CREAT,0600,0);
	semaforo[3] = sem_open("turnoClienteFIFO",O_CREAT,0600,0);
}

void liberar_recursos(int signum){
	int valorServidor = 12;
	sem_getvalue(semaforo[0],&valorServidor);
	if(valorServidor == 1){ //significa que el servidor le envio la señal kill por lo tanto este mismo sera el encargado de eliminar todo recurso.
		sem_close(semaforo[0]);
		sem_close(semaforo[1]);
		sem_close(semaforo[2]);
		sem_close(semaforo[3]);
		sem_unlink("clienteFIFO");
		sem_unlink("servidorFIFO");
		sem_unlink("turnoServFIFO");
		sem_unlink("turnoClienteFIFO");
		unlink(SEND_FIFO);
		unlink(RECEIVE_FIFO);
		shm_unlink("EJ3");
		exit(EXIT_SUCCESS);
	}else{	//significa que la señal enviada fue desde afuera, aqui sabemos que hay un servidor en ejecucion por lo que directamente le enviamos una señal para destruirlo.
		sem_close(semaforo[0]);
		sem_close(semaforo[2]);
		sem_close(semaforo[3]);
		int idAux = shm_open(MC, 0100 | 02, 0600);
    	dato *pidA = (dato*)mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, idAux,0);
    	close(idAux);
    	int pidServidor = pidA->pidServidor;
    	munmap(pidA,sizeof(dato));
		int value = 12;
		sem_getvalue(semaforo[1],&value);
		if(value == 0)
			sem_post(semaforo[1]);
		sem_close(semaforo[1]);
    	kill(pidServidor,SIGTERM);
		exit(EXIT_SUCCESS);
	}
}