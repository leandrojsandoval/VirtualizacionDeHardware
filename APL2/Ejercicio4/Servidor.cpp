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
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/file.h>

#define FILAS 4
#define COLUMNAS 4
#define TOTAL_PARES 8

const char *MEMORIA_COMPARTIDA = "/memoria_compartida"; // Asegúrate de que el nombre tenga una barra inclinada al principio
const char *SEM_CLIENTE = "/sem_cliente";
const char *SEM_SERVIDOR = "/sem_servidor";
const char *LOCK_FILE = "/tmp/servidor_memoria.lock";

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
                printf("- ");
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
        printf("Ejemplo de ejecucion:./cliente(terminal1) y ./servidor(terminal2)");
        printf("Ejemplo ingreso Fila-Columna en cliente, el mismo se va dar con la fila separada por un espacio columna: Fila Columna(0 1)\n");
        return true;
    }
    return false;
}

void manejarSIGUSR1(int signal)
{
    if (memoria->paresEncontrados == 0)
    {
        printf("Recibida señal SIGUSR1. Finalizando servidor...\n");
        munmap(memoria, sizeof(struct MemoriaCompartida));
        shm_unlink(MEMORIA_COMPARTIDA);
        sem_unlink(SEM_CLIENTE);
        sem_unlink(SEM_SERVIDOR);
        close(lock_fd);
        unlink(LOCK_FILE);
        exit(0);
    }
    else
    {
        printf("Recibida señal SIGUSR1, pero hay una partida en progreso. Ignorando...\n");
    }
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
        sem_close(sem_cliente);
        sem_unlink(SEM_CLIENTE);
        exit(EXIT_FAILURE);
    }
}

void limpiarRecursos()
{
    kill(memoria->cliente_pid, SIGUSR1); // Agrega esta línea
    if (memoria)
    {
        munmap(memoria, sizeof(struct MemoriaCompartida));
        shm_unlink(MEMORIA_COMPARTIDA);
    }
    if (sem_cliente)
    {
        sem_close(sem_cliente);
        sem_unlink(SEM_CLIENTE);
    }
    if (sem_servidor)
    {
        sem_close(sem_servidor);
        sem_unlink(SEM_SERVIDOR);
    }
    if (lock_fd != -1)
    {
        close(lock_fd);
        unlink(LOCK_FILE);
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        if (argc == 2 && (strcmp(argv[1], "-h") == 0 || (strcmp(argv[1], "--help") == 0)))
        {
            ayuda(argv[1]);
            exit(EXIT_SUCCESS);
        }
        else
        {
            printf("Error, el servidor no debe recibir parametros\n");
            printf("Consulte la ayuda con ./servidor -h\n");
            exit(EXIT_FAILURE);
        }
    }

    signal(SIGINT, manejarSIGINT);
    signal(SIGUSR1, manejarSIGUSR1);

    // Crear archivo de bloqueo
    lock_fd = open(LOCK_FILE, O_CREAT | O_RDWR, 0666);
    if (lock_fd == -1)
    {
        perror("Error al crear archivo de bloqueo");
        exit(EXIT_FAILURE);
    }
    if (flock(lock_fd, LOCK_EX | LOCK_NB) == -1)
    {
        perror("Error: otro servidor ya está en ejecución");
        close(lock_fd);
        exit(EXIT_FAILURE);
    }

    // Crear y mapear memoria compartida
    shm_fd = shm_open(MEMORIA_COMPARTIDA, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("Error al crear memoria compartida");
        limpiarRecursos();
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, sizeof(struct MemoriaCompartida)) == -1)
    {
        perror("Error al dimensionar la memoria compartida");
        limpiarRecursos();
        exit(EXIT_FAILURE);
    }
    memoria = (struct MemoriaCompartida *)mmap(NULL, sizeof(struct MemoriaCompartida), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memoria == MAP_FAILED)
    {
        perror("Error al mapear la memoria compartida");
        limpiarRecursos();
        exit(EXIT_FAILURE);
    }

    inicializarTablero();
    inicializarSemaforos();

    printf("Servidor iniciado. Esperando conexiones...\n");

    while (1)
    {
        sem_wait(sem_cliente); // Espera a que un cliente se conecte
        printf("Cliente conectado.\n");
        // mostrarTablero();

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
    limpiarRecursos();
   

    return 0;
}
