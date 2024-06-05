#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <cerrno>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <cstring>
#include <signal.h>
#include <string>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <vector>
#include <algorithm>
#include <atomic>


using namespace std;

typedef struct {
    int filaLetra1;
    int columnaLetra1;
    int filaLetra2;
    int columnaLetra2;
    int puntuacion;
} Adivinanza;

typedef struct {
    string respuesta;
    int puntuacion;
    vector<vector<char>> tableroActual;
} Respuesta;

// Utilizamos atomic para seguridad en hilos
atomic<bool> clientRunning(true);  

void signalHandler(int signum) {
    cout << "Interrupción recibida: " << signum << endl;
    clientRunning = false;
}

vector<char> recibirDatos(int socket, int size) {
    vector<char> buffer(size);
    int bytesRecibidos = recv(socket, buffer.data(), size, 0);
    if (bytesRecibidos <= 0) {
        cerr << "Error al recibir datos del servidor: " << strerror(errno) << endl;
        clientRunning = false;
    }
    buffer.resize(bytesRecibidos);
    return buffer;
}

Respuesta deserializarRespuesta(const vector<char>& data) {
    Respuesta resp;
    int offset = 0;

    // Deserializar 'respuesta'
    int respuestaLength;
    memcpy(&respuestaLength, data.data() + offset, sizeof(respuestaLength));
    offset += sizeof(respuestaLength);
    resp.respuesta = string(data.data() + offset, respuestaLength);
    offset += respuestaLength;

    // Deserializar 'puntuacion'
    memcpy(&resp.puntuacion, data.data() + offset, sizeof(resp.puntuacion));
    offset += sizeof(resp.puntuacion);

    // Deserializar 'tableroActual'
    resp.tableroActual.resize(4, vector<char>(4));
    for (auto& fila : resp.tableroActual) {
        for (auto& c : fila) {
            c = data[offset++];
        }
    }

    return resp;
}

void mostrarTablero(const vector<vector<char>>& tablero) {
    cout << "  0 1 2 3\n";
    for (int i = 0; i < 4; ++i) {
        cout << i << " ";
        for (int j = 0; j < 4; ++j) {
            cout << tablero[i][j] << " ";
        }
        cout << "\n";
    }
}

int main() {
    string serverIP;
    int serverPort;

    cout << "Ingrese la dirección IP del servidor: ";
    cin >> serverIP;
    cout << "Ingrese el puerto del servidor: ";
    cin >> serverPort;

    struct sockaddr_in serverConfig;
    int clienteSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clienteSocket == -1) {
        cerr << "Error al crear el socket: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    memset(&serverConfig, '0', sizeof(serverConfig));
    serverConfig.sin_family = AF_INET;
    serverConfig.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP.c_str(), &serverConfig.sin_addr) <= 0) {
        cerr << "Dirección IP inválida o no soportada" << endl;
        close(clienteSocket);
        exit(EXIT_FAILURE);
    }

    if (connect(clienteSocket, (struct sockaddr *)&serverConfig, sizeof(serverConfig)) < 0) {
        cerr << "Error al conectar al servidor: " << strerror(errno) << endl;
        close(clienteSocket);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, signalHandler);

    Adivinanza jugada;
    jugada.puntuacion = 0;
    while (clientRunning) {
        int fila1, col1, fila2, col2;
        cout << "Ingrese la fila y columna de la primera letra: ";
        cin >> fila1 >> col1;
        cout << "Ingrese la fila y columna de la segunda letra: ";
        cin >> fila2 >> col2;

        jugada = {fila1, col1, fila2, col2, jugada.puntuacion};

        // Enviar un latido al servidor para indicar que el cliente sigue activo
        string heartbeatMsg = "Heartbeat";
        send(clienteSocket, heartbeatMsg.c_str(), heartbeatMsg.length(), 0);

        if (send(clienteSocket, &jugada, sizeof(Adivinanza), 0) < 0) {
            cerr << "Error al enviar datos al servidor: " << strerror(errno) << endl;
            break;
        }

        vector<char> datosSerializados = recibirDatos(clienteSocket, 1024);
        if (!clientRunning) {
            break;
        }

        Respuesta respuesta = deserializarRespuesta(datosSerializados);
        cout << "Respuesta: " << respuesta.respuesta << endl;
        cout << "Puntuación: " << respuesta.puntuacion << endl;
        jugada.puntuacion = respuesta.puntuacion;
        mostrarTablero(respuesta.tableroActual);

        if (respuesta.respuesta == "El juego ha terminado. El ganador es el jugador con " + to_string(respuesta.puntuacion) + " puntos.") {
            cout << "El juego ha terminado." << endl;
            clientRunning = false;
        }
    }

    close(clienteSocket);
    cout << "Conexión cerrada." << endl;
    return 0;
}
