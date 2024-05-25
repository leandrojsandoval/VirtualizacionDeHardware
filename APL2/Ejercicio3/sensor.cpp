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
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FIFO "/tmp/sensor_fifo"

void ayuda ()
{
    cout << "============================== Ejercicio3 ==============================" << endl;
    cout << "Este script simular un sistema de toma de mediciones de sensores en una fábrica:" << std::endl;
    
}

void run_sensor(int sensor_number, int interval, int message_count) {
    int fifo_fd = open(FIFO, O_WRONLY);
    if (fifo_fd == -1) {
        std::cerr << "FIFO no existe, asegurarse de que el proceso central esté en ejecución." << std::endl;
        exit(1);
    }

    srand(time(0) + sensor_number);

    for (int i = 0; i < message_count; ++i) {
        int measurement = rand() % 101;
        std::string message = "Sensor " + std::to_string(sensor_number) + ": " + std::to_string(measurement) + "\n";
        write(fifo_fd, message.c_str(), message.size());
        sleep(interval);
    }

    close(fifo_fd);
}

int main(int argc, char* argv[]) {
    if (argc != 9 ||
        std::string(argv[1]) != "-n" ||
        std::string(argv[3]) != "-s" ||
        std::string(argv[5]) != "-m") {
        std::cerr << "Uso: " << argv[0] << " -n <numero_sensor> -s <intervalo_segundos> -m <cantidad_mensajes>" << std::endl;
        return 1;
    }

    int sensor_number = std::stoi(argv[2]);
    int interval = std::stoi(argv[4]);
    int message_count = std::stoi(argv[6]);

    run_sensor(sensor_number, interval, message_count);
    return 0;
}