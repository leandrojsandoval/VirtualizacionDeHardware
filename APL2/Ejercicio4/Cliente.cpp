#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>
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

struct MemoriaCompartida *memoria;
int shm_fd;
sem_t *sem_cliente, *sem_servidor;

void manejarSIGINT(int signal)
{
    // Ignorar SIGINT
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
int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        if ((strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
        {
            ayuda(argv[1]);
            exit(EXIT_SUCCESS);
        }
        else
        {
            printf("Error, el cliente no debe recibir parametros");
            exit(EXIT_FAILURE);
        }
    }
    signal(SIGINT, manejarSIGINT);

    // Abrir y mapear memoria compartida
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

    time_t start, end;
    double elapsed;
    time(&start);

    sem_post(sem_cliente); // Conectar al servidor

    while (1)
    {
        mostrarTablero();
        int fila1, col1, fila2, col2;
        printf("Ingrese la primera casilla (fila y columna): ");
        scanf("%d %d", &fila1, &col1);
        printf("Ingrese la segunda casilla (fila y columna): ");
        scanf("%d %d", &fila2, &col2);

        memoria->jugadas[0][0] = fila1;
        memoria->jugadas[0][1] = col1;
        memoria->jugadas[1][0] = fila2;
        memoria->jugadas[1][1] = col2;

        sem_post(sem_cliente); // Informar al servidor de la jugada

        sem_wait(sem_servidor); // Esperar a que el servidor procese la jugada

        // Mostrar el tablero actualizado
        mostrarTablero();

        if (memoria->paresEncontrados == TOTAL_PARES)
        {
            printf("¡Juego terminado! Todos los pares encontrados.\n");
            break;
        }
    }

    time(&end);
    elapsed = difftime(end, start);
    printf("Tiempo tomado: %.2f segundos\n", elapsed);

    sem_post(sem_cliente); // Desconectar del servidor

    munmap(memoria, sizeof(struct MemoriaCompartida));

    return 0;
}
