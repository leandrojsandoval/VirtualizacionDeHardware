/*
#########################################################
#               Virtualizacion de hardware              #
#                                                       #
#   APL2 - Ejercicio 1                                  #
#   Nombre del script: Ejercicio1.cpp                   #
#                                                       #
#   Integrantes:                                        #
#                                                       #
#       Ocampo, Nicole Fabiana              44451238    #
#       Sandoval Vasquez, Juan Leandro      41548235    #
#       Tigani, Martin Sebastian            32788835    #
#       Villegas, Lucas Ezequiel            37792844    #
#       Vivas, Pablo Ezequiel               38703964    #
#                                                       #
#   Instancia de entrega: Primera Entrega               #
#                                                       #
#########################################################
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <cstring>
#include <time.h>
#include <sys/file.h>

#define FILAS 4
#define COLUMNAS 4
#define TOTAL_PARES 8

const char *MEMORIA_COMPARTIDA = "/memoria_compartida";
const char *SEM_CLIENTE = "/sem_cliente";
const char *SEM_SERVIDOR = "/sem_servidor";
const char *LOCK_FILE = "/tmp/cliente_memoria.lock";

struct MemoriaCompartida
{
    char tablero[FILAS][COLUMNAS];
    int descubierto[FILAS][COLUMNAS];
    int paresEncontrados;
    int jugadas[2][2]; // Para almacenar las coordenadas de las jugadas del cliente
    pid_t cliente_pid; // Agrega esta línea
};

int shm_fd;
int lock_fd;
struct MemoriaCompartida *memoria;
sem_t *sem_cliente, *sem_servidor;

void manejarSIGUSR1(int signal)
{
    printf("Recibida señal SIGUSR1. Finalizando cliente...\n");
    exit(0);
}

void mostrarTablero()
{
    for (int i = 0; i < FILAS; i++)
    {
        for (int j = 0; j < COLUMNAS; j++)
        {
            if (memoria->descubierto[i][j])
            {
                printf("%c ", memoria->tablero[i][j]);
            }
            else
            {
                printf("- ");
            }
        }
        printf("\n");
    }
}

int leerCoordenada(int *fila, int *columna)
{
    char input[100];
    int leidos;

    if (fgets(input, sizeof(input), stdin) != NULL)
    {
        leidos = sscanf(input, "%d %d", fila, columna);
        if (leidos == 2)
        {
            if (*fila >= 0 && *fila <= 3 && *columna >= 0 && *columna <= 3)
            {
                return 1; // Se leyeron dos números válidos
            }else
                return 0;
            //return 1; // Se leyeron dos números
        }
    }
    return 0; // No se leyeron dos números
}
int main(int argc, char *argv[])
{
    signal(SIGUSR1, manejarSIGUSR1);

    lock_fd = open(LOCK_FILE, O_CREAT | O_RDWR, 0666);
    if (lock_fd == -1)
    {
        perror("Error al crear archivo de bloqueo");
        exit(EXIT_FAILURE);
    }
    if (flock(lock_fd, LOCK_EX | LOCK_NB) == -1)
    {
        perror("Error: otro cliente ya está en ejecución");
        close(lock_fd);
        exit(EXIT_FAILURE);
    }

    // Abrir memoria compartida
    shm_fd = shm_open(MEMORIA_COMPARTIDA, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error al abrir memoria compartida");
        exit(EXIT_FAILURE);
    }

    memoria = (struct MemoriaCompartida *)mmap(NULL, sizeof(struct MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memoria == MAP_FAILED)
    {
        perror("Error al mapear la memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Abrir semáforos
    sem_cliente = sem_open(SEM_CLIENTE, 0);
    if (sem_cliente == SEM_FAILED)
    {
        perror("Error al abrir semáforo cliente");
        exit(EXIT_FAILURE);
    }

    sem_servidor = sem_open(SEM_SERVIDOR, 0);
    if (sem_servidor == SEM_FAILED)
    {
        perror("Error al abrir semáforo servidor");
        exit(EXIT_FAILURE);
    }

    memoria->cliente_pid = getpid(); // Agrega esta línea
    time_t start, end;
    double elapsed;
    time(&start);
    while (1)
    {
        time(&end);
        elapsed = difftime(end, start);
        printf("Tiempo tomado: %.2f segundos\n", elapsed);
        mostrarTablero();
        /*
        printf("Ingrese las coordenadas de la primera carta (fila columna): ");
        scanf("%d %d", &memoria->jugadas[0][0], &memoria->jugadas[0][1]);
        printf("Ingrese las coordenadas de la segunda carta (fila columna): ");
        scanf("%d %d", &memoria->jugadas[1][0], &memoria->jugadas[1][1]); */

        do
        {
            printf("Ingrese las coordenadas de la primera (fila columna): ");
        } while (!leerCoordenada(&memoria->jugadas[0][0], &memoria->jugadas[0][1]));

        do
        {
            printf("Ingrese las coordenadas de la segunda (fila columna): ");
        } while (!leerCoordenada(&memoria->jugadas[1][0], &memoria->jugadas[1][1]) ||
                 (memoria->jugadas[0][0] == memoria->jugadas[1][0] && memoria->jugadas[0][1] == memoria->jugadas[1][1]));

        sem_post(sem_cliente);  // Notifica al servidor que el cliente ha hecho una jugada
        sem_wait(sem_servidor); // Espera a que el servidor procese la jugada

        // Verifica si el juego ha terminado
        if (memoria->paresEncontrados == TOTAL_PARES)
        {
            printf("¡Felicitaciones! Encontraste todos los pares.\n");
            break;
        }
    }

    // Cerrar recursos
    munmap(memoria, sizeof(struct MemoriaCompartida));
    close(shm_fd);
    sem_close(sem_cliente);
    sem_close(sem_servidor);

    return 0;
}
