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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

using namespace std;

const char* FIFO_NAME = "/tmp/sensor_fifo";

// int main(int argc, char* argv[]) {

//     int sensorNumber = -1;
//     int interval = -1;
//     int messageCount = -1;

//     // Recorre los argumentos de línea de comandos
//     for (int i = 1; i < argc; ++i) {
//         // Comprueba si el argumento actual es "-n" y que haya un siguiente argumento
//         if (string(argv[i]) == "-n" && i + 1 < argc) {
//             sensorNumber = stoi(argv[++i]); // Asigna el siguiente argumento a sensorNumber
//         } else if (string(argv[i]) == "-s" && i + 1 < argc) {
//             interval = stoi(argv[++i]); // Asigna el siguiente argumento a interval
//         } else if (string(argv[i]) == "-m" && i + 1 < argc) {
//             messageCount = stoi(argv[++i]); // Asigna el siguiente argumento a messageCount
//         }
//     }


//      // Comprueba que todos los parámetros requeridos hayan sido proporcionados
//     if (sensorNumber == -1 || interval == -1 || messageCount == -1) {
//         cerr << "Usage: " << argv[0] << " -n <sensor number> -s <seconds interval> -m <messages count>" << endl;
//         return 1;
//     }

//     //Se abre el FIFO en modo escritura (O_WRONLY).
//     int fd = open(FIFO_NAME, O_WRONLY);
//     if (fd == -1) {
//         cerr << "Failed to open FIFO" << endl;
//         return 1;
//     }

//     // Inicializa el generador de números aleatorios
//     srand(time(nullptr));

//     // Bucle para enviar las mediciones
//     for (int i = 0; i < messageCount; ++i) {
//         int measurement = rand() % 100; // Genera una medición aleatoria
//         string message = "Sensor " + to_string(sensorNumber) + ": " + to_string(measurement);
//         write(fd, message.c_str(), message.size()); // Escribe el mensaje en el FIFO
//         //sleep(interval); // Espera el intervalo especificado
//     }

//     // Cierra el FIFO
//     close(fd);
//     return 0;
// }

int main(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Usage: %s -n <sensor_number> -s <seconds> -m <messages>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sensor_number = -1;
    int seconds = -1;
    int messages = -1;

    int opt;
    while ((opt = getopt(argc, argv, "n:s:m:")) != -1) {
        switch (opt) {
            case 'n':
                sensor_number = atoi(optarg);
                break;
            case 's':
                seconds = atoi(optarg);
                break;
            case 'm':
                messages = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -n <sensor_number> -s <seconds> -m <messages>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (sensor_number == -1 || seconds == -1 || messages == -1) {
        fprintf(stderr, "All parameters are required\n");
        exit(EXIT_FAILURE);
    }

    int fd = open(FIFO_NAME, O_WRONLY);
    if (fd == -1) {
        perror("Error opening FIFO");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL) + sensor_number);

    for (int i = 0; i < messages; ++i) {
        int measurement = rand() % 100; // Simulación de medición

        char message[100];
        snprintf(message, sizeof(message), "Sensor %d: %d", sensor_number, measurement);

        write(fd, message, strlen(message) + 1);
        sleep(seconds);
    }

    close(fd);
    return 0;
}