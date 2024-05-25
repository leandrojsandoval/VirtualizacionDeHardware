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

#define FIFO "/tmp/sensor_fifo"

std::ofstream logFile;

void signal_handler(int signum) {
    std::cout << "Señal de terminación recibida, cerrando demonio." << std::endl;
    if (std::remove(FIFO) != 0) {
        perror("Error eliminando el FIFO");
    }
    logFile.close();
    exit(signum);
}

void run_daemon(const char* log_file_path) {
    logFile.open(log_file_path, std::ios_base::app);
    if (!logFile.is_open()) {
        std::cerr << "Error abriendo el archivo de log." << std::endl;
        exit(1);
    }

    if (mkfifo(FIFO, 0666) == -1 && errno != EEXIST) {
        perror("Error creando el FIFO");
        exit(1);
    }

    std::cout << "Daemon iniciado, esperando mensajes..." << std::endl;
    std::signal(SIGTERM, signal_handler);

    int fifo_fd = open(FIFO, O_RDONLY);
    if (fifo_fd == -1) {
        perror("Error abriendo el FIFO");
        exit(1);
    }

    char buffer[256];
    while (true) {
        ssize_t bytesRead = read(fifo_fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            time_t now = time(0);
            tm *ltm = localtime(&now);
            logFile << 1900 + ltm->tm_year << "-"
                    << 1 + ltm->tm_mon << "-"
                    << ltm->tm_mday << " "
                    << 1 + ltm->tm_hour << ":"
                    << 1 + ltm->tm_min << ":"
                    << 1 + ltm->tm_sec << " "
                    << buffer << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3 || std::string(argv[1]) != "-l") {
        std::cerr << "Uso: " << argv[0] << " -l <archivo_log>" << std::endl;
        return 1;
    }

    run_daemon(argv[2]);
    return 0;
}