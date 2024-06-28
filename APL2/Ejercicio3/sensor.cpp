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
#include <getopt.h>
#include <ctype.h>

using namespace std;

const char* FIFO_NAME = "/tmp/sensor_fifo";

void print_help() {
    printf("Usage: sensor [OPTIONS]\n");
    printf("Options:\n");
    printf("  -n, --numero <sensor_number>    Sensor number (integer)\n");
    printf("  -s, --segundos <seconds>        Interval in seconds (integer)\n");
    printf("  -m, --mensajes <messages>       Number of messages (integer)\n");
    printf("  -h, --help                      Display this help message\n");
}

int is_numeric(const char *str) {
    for (int i = 0; str[i] != '\0'; ++i) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}


int main(int argc, char *argv[]) {

    int sensor_number = -1;
    int seconds = -1;
    int messages = -1;

    const struct option long_options[] = {
        {"numero", required_argument, 0, 'n'},
        {"segundos", required_argument, 0, 's'},
        {"mensajes", required_argument, 0, 'm'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int long_index = 0;
    while ((opt = getopt_long(argc, argv, "n:s:m:h", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'n':
                if (!is_numeric(optarg)) {
                    fprintf(stderr, "Sensor number must be a positive integer\n");
                    print_help();
                    exit(EXIT_FAILURE);
                }
                sensor_number = atoi(optarg);
                break;
            case 's':
                if (!is_numeric(optarg)) {
                    fprintf(stderr, "Seconds must be a positive integer\n");
                    print_help();
                    exit(EXIT_FAILURE);
                }
                seconds = atoi(optarg);
                break;
            case 'm':
                if (!is_numeric(optarg)) {
                    fprintf(stderr, "Messages must be a positive integer\n");
                    print_help();
                    exit(EXIT_FAILURE);
                }
                messages = atoi(optarg);
                break;
            case 'h':
            default:
                print_help();
                exit(EXIT_FAILURE);
        }
    }

    if (sensor_number <= 0 || seconds <= 0 || messages <= 0) {
        fprintf(stderr, "All parameters must be positive integers\n");
        print_help();
        exit(EXIT_FAILURE);
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