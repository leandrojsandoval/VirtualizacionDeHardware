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

#include <iostream>
#include <fstream>
#include <csignal>
#include <ctime>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>


using namespace std;

const char* FIFO_NAME = "/tmp/sensor_fifo";
ofstream logFile;

int keep_running = 1;

void handle_signal(int sig) {
    keep_running = 0;
}

void cleanup() {
    unlink(FIFO_NAME); // Elimina el FIFO
}


void write_log(const char *logfile, const char *message) {
    FILE *file = fopen(logfile, "a");
    if (file == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%s\n", message);
    fclose(file);
}

int main(int argc, char *argv[]) {

    cout << "PID del proceso: " << getpid() << endl;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s -l <logfile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *logfile = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "l:")) != -1) {
        switch (opt) {
            case 'l':
                logfile = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -l <logfile>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (logfile == NULL) {
        fprintf(stderr, "Log file is required\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    unlink(FIFO_NAME);

    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("Error creating FIFO");
        exit(EXIT_FAILURE);
    }

    // Registrar la función de limpieza para que se ejecute al finalizar
    atexit(cleanup);
    
    int fd;
    char buffer[256];

    while (keep_running) {
        fd = open(FIFO_NAME, O_RDONLY | O_NONBLOCK);
        if (fd == -1) {
            perror("Error opening FIFO");
            exit(EXIT_FAILURE);
        }

        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;

        int poll_ret = poll(&pfd, 1, 1000); // Esperar hasta 1 segundo

        if (poll_ret == -1) {
            perror("Proceso demonio terminado ");
            close(fd);
            exit(EXIT_FAILURE);
        } else if (poll_ret == 0) {
            // Timeout, continuar el bucle
            close(fd);
            continue;
        }

        if (pfd.revents & POLLIN) {
            ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';

                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                char time_str[100];
                strftime(time_str, sizeof(time_str) - 1, "%Y-%m-%d %H:%M:%S", t);

                char log_entry[512];
                snprintf(log_entry, sizeof(log_entry), "%s - %s", time_str, buffer);
                write_log(logfile, log_entry);
            }
        }

        close(fd);
    }
     
    
    unlink(FIFO_NAME);
    return 0;
}
