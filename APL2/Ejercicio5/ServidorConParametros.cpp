#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <getopt.h>
#include <pthread.h>

using namespace std;

#define BUFFER_SIZE 16384

struct Jugador {
    int idJugador;
    int puntuacion;
    int socket;
    string nickname;
};

struct Adivinanza {
    int filaLetra1;
    int columnaLetra1;
    int filaLetra2;
    int columnaLetra2;
};

struct Consulta {
    vector<vector<char>> tableroActual;
    char consulta[256];
};

struct Respuesta {
    vector<vector<char>> tableroActual;
    char respuesta[256];
};

vector<Jugador> clientesSockets;

void *manejadorCliente(void *arg);
vector<vector<char>> generarTablero();
vector<vector<char>> mostrarTablero(const vector<vector<char>>& tablero, const vector<vector<bool>>& descubiertas);
bool tableroCompleto(const vector<vector<bool>>& descubiertas);
string comprobarYActualizarTablero(vector<vector<char>>& tablero, vector<vector<bool>>& descubiertas, int fila1, int col1, int fila2, int col2, int idJugador);
void actualizarPuntuacion(int idJugador, int puntos);
void mostrarTableroEnServidor(const vector<vector<char>>& tablero);
void agregarJugador(int idJugador, int puntuacion, int socket, const string& nickname);
void imprimirJugadores();
void eliminarJugador(int idJugador);
Adivinanza deserializarAdivinanza(const vector<char> &buffer);
vector<char> serializarConsulta(const Consulta &con);
vector<char> serializarRespuesta(const Respuesta &resp);
void enviarBuffer(int socket, const vector<char> &buffer);
vector<char> recibirBuffer(int socket);
pair<int, list<int>> obtenerGanadores();

int main(int argc, char *argv[]) {
    int port = 0;
    int num_jugadores = 0;

    const char *optstring = "p:j:";
    struct option longopts[] = {
        {"puerto", required_argument, NULL, 'p'},
        {"jugadores", required_argument, NULL, 'j'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'j':
                num_jugadores = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Uso: %s -p <puerto> -j <jugadores>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (port == 0 || num_jugadores == 0) {
        fprintf(stderr, "Uso: %s -p <puerto> -j <jugadores>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    vector<vector<char>> tablero = generarTablero();
    vector<vector<bool>> descubiertas(4, vector<bool>(4, false));

    // Crear socket de servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Asignar dirección y puerto al socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, num_jugadores) < 0) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }

    cout << "Servidor en espera de " << num_jugadores << " jugadores en el puerto " << port << "...\n";

    while (clientesSockets.size() < num_jugadores) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Error en accept");
            exit(EXIT_FAILURE);
        }

        // Recibir nickname del cliente
        char nickname[256];
        read(new_socket, nickname, 256);

        int idJugador = clientesSockets.size() + 1;
        agregarJugador(idJugador, 0, new_socket, nickname);

        cout << "Jugador " << idJugador << " (" << nickname << ") conectado\n";
        imprimirJugadores();

        pthread_t tid;
        if (pthread_create(&tid, NULL, manejadorCliente, &new_socket) != 0) {
            perror("Error al crear hilo");
            exit(EXIT_FAILURE);
        }
    }

    cout << "Todos los jugadores se han conectado. Iniciando el juego...\n";

    while (!tableroCompleto(descubiertas)) {
        for (auto& cliente : clientesSockets) {
            Consulta consulta;
            consulta.tableroActual = mostrarTablero(tablero, descubiertas);
            strcpy(consulta.consulta, "Elige dos letras");

            vector<char> bufferConsulta = serializarConsulta(consulta);
            enviarBuffer(cliente.socket, bufferConsulta);

            vector<char> bufferAdivinanza = recibirBuffer(cliente.socket);
            Adivinanza adivinanza = deserializarAdivinanza(bufferAdivinanza);

            string resultado = comprobarYActualizarTablero(tablero, descubiertas, adivinanza.filaLetra1, adivinanza.columnaLetra1, adivinanza.filaLetra2, adivinanza.columnaLetra2, cliente.idJugador);

            Respuesta respuesta;
            respuesta.tableroActual = mostrarTablero(tablero, descubiertas);
            strcpy(respuesta.respuesta, resultado.c_str());

            vector<char> bufferRespuesta = serializarRespuesta(respuesta);
            enviarBuffer(cliente.socket, bufferRespuesta);
        }
    }

    auto [maxPuntos, ganadores] = obtenerGanadores();

    cout << "Juego terminado. Ganadores:\n";
    for (int ganador : ganadores) {
        cout << "Jugador " << ganador << " con " << maxPuntos << " puntos\n";
    }

    close(server_fd);
    return 0;
}

vector<vector<char>> generarTablero() {
    vector<vector<char>> tablero(4, vector<char>(4));
    vector<char> letras = {'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D', 'E', 'E', 'F', 'F', 'G', 'G', 'H', 'H'};
    random_shuffle(letras.begin(), letras.end());

    int k = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            tablero[i][j] = letras[k++];
        }
    }
    return tablero;
}

vector<vector<char>> mostrarTablero(const vector<vector<char>>& tablero, const vector<vector<bool>>& descubiertas) {
    vector<vector<char>> tableroMostrado = tablero;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!descubiertas[i][j]) {
                tableroMostrado[i][j] = '-';
            }
        }
    }
    return tableroMostrado;
}

bool tableroCompleto(const vector<vector<bool>>& descubiertas) {
    for (const auto& fila : descubiertas) {
        for (bool descubierta : fila) {
            if (!descubierta) return false;
        }
    }
    return true;
}

string comprobarYActualizarTablero(vector<vector<char>>& tablero, vector<vector<bool>>& descubiertas, int fila1, int col1, int fila2, int col2, int idJugador) {
    if (tablero[fila1][col1] == tablero[fila2][col2]) {
        descubiertas[fila1][col1] = true;
        descubiertas[fila2][col2] = true;
        actualizarPuntuacion(idJugador, 1);
        return "¡Adivinanza correcta!";
    } else {
        return "Adivinanza incorrecta, intenta de nuevo.";
    }
}

void actualizarPuntuacion(int idJugador, int puntos) {
    for (auto& jugador : clientesSockets) {
        if (jugador.idJugador == idJugador) {
            jugador.puntuacion += puntos;
            break;
        }
    }
}

void mostrarTableroEnServidor(const vector<vector<char>>& tablero) {
    for (const auto& fila : tablero) {
        for (char letra : fila) {
            cout << letra << " ";
        }
        cout << endl;
    }
}

void agregarJugador(int idJugador, int puntuacion, int socket, const string& nickname) {
    clientesSockets.push_back({idJugador, puntuacion, socket, nickname});
}

void imprimirJugadores() {
    for (const auto& jugador : clientesSockets) {
        cout << "Jugador " << jugador.idJugador << " (" << jugador.nickname << ") - Puntuación: " << jugador.puntuacion << "\n";
    }
}

void eliminarJugador(int idJugador) {
    clientesSockets.erase(remove_if(clientesSockets.begin(), clientesSockets.end(), [&](Jugador& j) {
        return j.idJugador == idJugador;
    }), clientesSockets.end());
}

Adivinanza deserializarAdivinanza(const vector<char>& buffer) {
    Adivinanza adivinanza;
    memcpy(&adivinanza, buffer.data(), sizeof(Adivinanza));
    return adivinanza;
}

vector<char> serializarConsulta(const Consulta &con) {
    vector<char> buffer;
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(&con.tableroActual[0]), reinterpret_cast<const char*>(&con.tableroActual[0]) + con.tableroActual.size() * con.tableroActual[0].size() * sizeof(char));
    buffer.insert(buffer.end(), con.consulta, con.consulta + strlen(con.consulta) + 1);
    return buffer;
}

vector<char> serializarRespuesta(const Respuesta &resp) {
    vector<char> buffer;
    buffer.insert(buffer.end(), reinterpret_cast<const char*>(&resp.tableroActual[0]), reinterpret_cast<const char*>(&resp.tableroActual[0]) + resp.tableroActual.size() * resp.tableroActual[0].size() * sizeof(char));
    buffer.insert(buffer.end(), resp.respuesta, resp.respuesta + strlen(resp.respuesta) + 1);
    return buffer;
}

void enviarBuffer(int socket, const vector<char>& buffer) {
    int lenBuffer = buffer.size();
    send(socket, reinterpret_cast<const char*>(&lenBuffer), sizeof(lenBuffer), 0);
    send(socket, buffer.data(), buffer.size(), 0);
}

vector<char> recibirBuffer(int socket) {
    int lenBuffer;
    recv(socket, reinterpret_cast<char*>(&lenBuffer), sizeof(lenBuffer), 0);
    vector<char> buffer(lenBuffer);
    recv(socket, buffer.data(), buffer.size(), 0);
    return buffer;
}

pair<int, list<int>> obtenerGanadores() {
    int maxPuntos = 0;
    for (const auto& jugador : clientesSockets) {
        if (jugador.puntuacion > maxPuntos) {
            maxPuntos = jugador.puntuacion;
        }
    }
    list<int> ganadores;
    for (const auto& jugador : clientesSockets) {
        if (jugador.puntuacion == maxPuntos) {
            ganadores.push_back(jugador.idJugador);
        }
    }
    return {maxPuntos, ganadores};
}

void *manejadorCliente(void *arg) {
    int socket = *((int *)arg);

    while (true) {
        vector<char> bufferConsulta = recibirBuffer(socket);
        Consulta consulta = deserializarConsulta(bufferConsulta);

        // Aquí podrías procesar la consulta en función de la lógica del juego
    }

    close(socket);
    return nullptr;
}
