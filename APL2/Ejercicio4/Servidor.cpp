#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <cstring>

#define FILAS 4
#define COLUMNAS 4
#define TOTAL_PARES 8

const char *MEMORIA_COMPARTIDA = "memoria_compartida";
const char *SEM_CLIENTE = "/sem_cliente";
const char *SEM_SERVIDOR = "/sem_servidor";

struct MemoriaCompartida
{
    char tablero[FILAS][COLUMNAS];
    int descubierto[FILAS][COLUMNAS];
    int paresEncontrados;
    int jugadas[2][2]; // Para almacenar las coordenadas de las jugadas del cliente
};

int shm_fd;
struct MemoriaCompartida *memoria;
sem_t *sem_cliente, *sem_servidor;

void inicializarTablero()
{
    char letras[TOTAL_PARES * 2];
    char letra = 'A';
    for (int i = 0; i < TOTAL_PARES; i++)
    {
        letras[2 * i] = letra;
        letras[2 * i + 1] = letra;
        letra++;
    }
    srand(time(NULL));
    for (int i = 0; i < FILAS * COLUMNAS; i++)
    {
        int j = rand() % (FILAS * COLUMNAS);
        char temp = letras[i];
        letras[i] = letras[j];
        letras[j] = temp;
    }
    for (int i = 0; i < FILAS; i++)
    {
        for (int j = 0; j < COLUMNAS; j++)
        {
            memoria->tablero[i][j] = letras[i * COLUMNAS + j];
            memoria->descubierto[i][j] = 0;
        }
    }
    memoria->paresEncontrados = 0;
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
                printf("* ");
            }
        }
        printf("\n");
    }
}
bool ayuda(const char *cad)
{
    if (!strcmp(cad, "-h") || !strcmp(cad, "--help"))
    {
        printf("Esta script permite Implementar el clásico juego de la memoria Memotest, pero alfabético.\n");
        printf("Deberá existir un proceso Cliente, cuya tarea será mostrar por pantalla el estado actual del tablero y leer desde teclado el par de casillas que el usuario quiere destapar\n");
        printf("También existirá un proceso Servidor, que será el encargado de actualizar el estado del tablero en base al par de casillas ingresado,así como controlar la finalización partida");
        return true;
    }
    return false;
}
void manejarSIGUSR1(int signal)
{
    printf("Recibida señal SIGUSR1. Finalizando servidor...\n");
    munmap(memoria, sizeof(struct MemoriaCompartida));
    shm_unlink(MEMORIA_COMPARTIDA);
    sem_unlink(SEM_CLIENTE);
    sem_unlink(SEM_SERVIDOR);
    exit(0);
}

void manejarSIGINT(int signal)
{
    printf("Recibida señal SIGINT. Ignorando...\n");
}

void inicializarSemaforos()
{
    sem_cliente = sem_open(SEM_CLIENTE, O_CREAT, 0666, 0);
    if (sem_cliente == SEM_FAILED)
    {
        perror("Error al crear semáforo cliente");
        exit(EXIT_FAILURE);
    }

    sem_servidor = sem_open(SEM_SERVIDOR, O_CREAT, 0666, 0);
    if (sem_servidor == SEM_FAILED)
    {
        perror("Error al crear semáforo servidor");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        if ((strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) )
        {
            ayuda(argv[1]);
            exit(EXIT_SUCCESS);
        }
        else
        {
            printf("Error, el servidor no debe recibir parametros");
            exit(EXIT_FAILURE);
        }
    }

    signal(SIGINT, manejarSIGINT);
    signal(SIGUSR1, manejarSIGUSR1);

    // Crear y mapear memoria compartida
    shm_fd = shm_open(MEMORIA_COMPARTIDA, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error al crear memoria compartida");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, sizeof(struct MemoriaCompartida)) == -1)
    {
        perror("Error al dimensionar la memoria compartida");
        exit(EXIT_FAILURE);
    }
    memoria = (struct MemoriaCompartida *)mmap(NULL, sizeof(struct MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memoria == MAP_FAILED)
    {
        perror("Error al mapear la memoria compartida");
        exit(EXIT_FAILURE);
    }

    inicializarTablero();
    inicializarSemaforos();

    printf("Servidor iniciado. Esperando conexiones...\n");

    while (1)
    {
        sem_wait(sem_cliente); // Espera a que un cliente se conecte
        printf("Cliente conectado.\n");
        mostrarTablero();

        // Procesamiento del juego aquí
        int fila1 = memoria->jugadas[0][0];
        int col1 = memoria->jugadas[0][1];
        int fila2 = memoria->jugadas[1][0];
        int col2 = memoria->jugadas[1][1];

        printf("Coordenadas de la jugada: (%d, %d) y (%d, %d)\n", fila1, col1, fila2, col2);

        if (memoria->tablero[fila1][col1] == memoria->tablero[fila2][col2])
        {
            memoria->descubierto[fila1][col1] = 1;
            memoria->descubierto[fila2][col2] = 1;
            memoria->paresEncontrados++;
        }

        printf("Pares encontrados hasta ahora: %d\n", memoria->paresEncontrados);

        // Finalizar el juego cuando se encuentren todos los pares
        if (memoria->paresEncontrados == TOTAL_PARES)
        {
            printf("Todos los pares encontrados. Juego terminado.\n");
            break;
        }
        sem_post(sem_servidor); // Permite al siguiente cliente conectarse
    }

    printf("Finalizando servidor...\n");

    munmap(memoria, sizeof(struct MemoriaCompartida));
    shm_unlink(MEMORIA_COMPARTIDA);
    sem_unlink(SEM_CLIENTE);
    sem_unlink(SEM_SERVIDOR);

    return 0;
}
