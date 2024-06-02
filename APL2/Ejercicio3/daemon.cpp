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


using namespace std;

const char* FIFO_NAME = "/tmp/sensor_fifo";
ofstream logFile;

void signalHandler(int signum) {
    cout << "Interrupt signal (" << signum << ") received. Shutting down." << endl;
    logFile.close();
    unlink(FIFO_NAME); // Borra el FIFO
    exit(signum);
}

//obtener fecha y hora actual
string currentDateTime() {
    time_t now = time(0);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", localtime(&now));
    return buf;
}

int main(int argc, char* argv[]) {
    if (argc != 3 || string(argv[1]) != "-l") {
        cerr << "Usage: " << argv[0] << " -l <logfile>" << endl;
        return 1;
    }

    string logFileName = argv[2];

    // Daemonize the process
    pid_t pid = fork(); //crea un nuevo proceso
    if (pid < 0) {
        cerr << "Fork failed!" << endl;
        return 1;
    }
    if (pid > 0) {
        cout << "Daemon PID: " << pid << endl;
        exit(0);
    }

    umask(0);
    pid_t sid = setsid(); //crea un nueva sesion para el proceso hijo
    if (sid < 0) {
        return 1;
    }

    if ((chdir("/")) < 0) { //cambia el directorio de trabajo a la raiz
        return 1;
    }

    //Se cierran los descriptores de archivo estándar (stdin, stdout, stderr).
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    signal(SIGINT, signalHandler);

    mkfifo(FIFO_NAME, 0666); //Se crea el FIFO con mkfifo() y se abre el archivo de log.

    logFile.open(logFileName, std::ios_base::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file" << std::endl;
        return 1;
    }

    int fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        std::cerr << "Failed to open FIFO" << std::endl;
        return 1;
    }

    char buffer[128];
    while (true) {
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            logFile << currentDateTime() << " " << buffer << std::endl;
        }
    }

    close(fd);
    logFile.close();
    unlink(FIFO_NAME);

    return 0;
}
