#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <list>
#include <cstring>
#include <string.h>
#include <cstring>
#include <sstream>

using namespace std;

#define BUFFER_SIZE 16384

typedef struct {
    int pidServ;
    int Socket_Escucha;
} dato;

typedef struct {
    int filaLetra1;
    int columnaLetra1;
    int filaLetra2;
    int columnaLetra2;
} Adivinanza;

typedef struct {
    vector<vector<char>> tableroActual;
    char *respuesta;
} Respuesta;

typedef struct{
    vector<vector<char>> tableroActual;
    char *consulta;
}Consulta;

typedef struct {
    int puntuacion;
    int idJugador;
    char nickname[50];  // Ajustar el tamaño según se necesite
} Jugador;

list<Jugador> clientesSockets;

/*******************Funciones particulares del caso***************************/
vector<vector<char>> generarTablero();
vector<vector<char>> mostrarTablero(const vector<vector<char>>& tablero, const vector<vector<bool>>& descubiertas);
bool tableroCompleto(const vector<vector<bool>>& descubiertas);
string comprobarYActualizarTablero(vector<vector<char>>& tablero, vector<vector<bool>>& descubiertas, int fila1, int col1, int fila2, int col2,int idJugador);
void actualizarPuntuacion(int idJugador, int puntos);

void mostrarTableroEnServidor(const vector<vector<char>>& tablero);
void agregarJugador(int idJugador, int puntuacion);
void imprimirJugadores();
void eliminarJugador(int idJugador);

Adivinanza deserializarAdivinanza(const vector<char> &buffer);
vector<char> serializarConsulta(const Consulta &con);
vector<char> serializarRespuesta(const Respuesta &resp);
void enviarBuffer(int socket, const vector<char> &buffer);
vector<char> recibirBuffer(int socket);

pair<int, list<int>> obtenerGanadores();

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <puerto> <cantidad_maxima_de_clientes>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int max_clients = atoi(argv[2]);
    int server_fd, new_socket, client_sockets[max_clients];
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Inicializar los sockets de cliente
    for (int i = 0; i < max_clients; i++) {
        client_sockets[i] = 0;
    }

    // Crear el socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Vincular el socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(server_fd, max_clients) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %d\n", port);

    // Aceptar conexiones hasta el límite de max_clients
    int connected_clients = 0;
    while (connected_clients < max_clients) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("Nuevo cliente conectado, socket fd: %d, IP: %s, puerto: %d\n",
               new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        agregarJugador(new_socket,0);
        connected_clients++;
    }

    imprimirJugadores();

 
    //generar tablero de partida
    srand(time(0)); // Semilla para el generador de números aleatorios
    vector<vector<char>> tablero_partida = generarTablero();
    mostrarTableroEnServidor(tablero_partida);
    //generar vector de descubiertos
    vector<vector<bool>> descubiertas(4, vector<bool>(4, false));

    // Notificar a cada cliente que el juego ha comenzado
    for (const auto& jugador : clientesSockets) {
        const char *message = "El juego ha comenzado\n";
        printf("Notificando al cliente %d\n",jugador.idJugador);
        send(jugador.idJugador, message, strlen(message), 0);
    }



    // Manejar los mensajes de los clientes en turnos
    while (!tableroCompleto(descubiertas) and !clientesSockets.empty()) {
        for (const auto& jugador : clientesSockets) {
            if(!tableroCompleto(descubiertas)){ 
                // Generar consulta actual para enviar al cliente. 
                Consulta con;
                con.tableroActual = mostrarTablero(tablero_partida, descubiertas);
                const char *consultaStr = "Es tu turno! ¿Dónde se encuentran coincidencias?\n";
                con.consulta = new char[strlen(consultaStr) + 1];
                strcpy(con.consulta, consultaStr); 
                
                // serializar consulta para enviarselo al cliente
                vector<char> consulta_serializada = serializarConsulta(con);


                // enviar consulta para el cliente
                enviarBuffer(jugador.idJugador, consulta_serializada);

                // recibir adivinanza del cliente y deserializar esta
                vector<char> adivinanza_recibida = recibirBuffer(jugador.idJugador);
                Adivinanza adivinanza = deserializarAdivinanza(adivinanza_recibida);

                // validar respuesta del cliente, es decir, si acertó o nó (validaremos que el cliente valide sus propios tipeos, es decir, que todo caracter enviado sea un entero en sí)
                string resultado = comprobarYActualizarTablero(tablero_partida, descubiertas, adivinanza.filaLetra1, adivinanza.columnaLetra1, adivinanza.filaLetra2, adivinanza.columnaLetra2, jugador.idJugador);

                // generar tablero actual en base al prosesamiento
                Respuesta resp;
                resp.tableroActual = mostrarTablero(tablero_partida, descubiertas);
                resp.respuesta = new char[resultado.length() + 1];
                strcpy(resp.respuesta, resultado.c_str());

                // serializar respuesta y enviar respuesta al jugador
                vector<char> respuesta_serializada = serializarRespuesta(resp);
                enviarBuffer(jugador.idJugador, respuesta_serializada);


                // limpiar memoria

                delete[] con.consulta;
                delete[] resp.respuesta;
            }
            else
                break;
        }
    }
    printf("Notificando finalización de juego\n");

    Consulta con_notificacion;
    con_notificacion.tableroActual = mostrarTablero(tablero_partida, descubiertas);
    const char *messageFinally = "El juego ha terminado\n";
    con_notificacion.consulta = new char[strlen(messageFinally) + 1];
    strcpy(con_notificacion.consulta, messageFinally);
    vector<char> consulta_serializada = serializarConsulta(con_notificacion);
    //notificar a clientes de juego terminado.
    for (const auto& jugador : clientesSockets)
        enviarBuffer(jugador.idJugador, consulta_serializada);

    // Obtener ganadores y puntaje máximo
    pair<int, list<int>> resultado = obtenerGanadores();
    int puntajeMaximo = resultado.first;
    list<int> ganadores = resultado.second;

    // Enviar el puntaje máximo y los ganadores a todos los jugadores
    stringstream ss;
    ss << "El puntaje máximo es: " << puntajeMaximo << ". ";
    if (ganadores.size() > 1) {
        ss << "Hay un empate entre los jugadores: ";
    } else {
        ss << "El ganador es el jugador: ";
    }
    for (int id : ganadores) {
        ss << id << " ";
    }
    ss << "\n";
    string mensajeFinal = ss.str();

    for (const auto& jugador : clientesSockets) {
        send(jugador.idJugador, mensajeFinal.c_str(), mensajeFinal.length(), 0);
    }
    // Cerrar sockets de clientes
    for (const auto& jugador : clientesSockets) {
        close(jugador.idJugador);
    }

    if(clientesSockets.empty()){
        //quehacer en estos casos?
    }

    // Cerrar el socket del servidor
    close(server_fd);

    //liberar recursos, eso cuando implemente lo del servidor en segundo plano...

    return 0;
}

vector<vector<char>> generarTablero() {
    vector<char> todasLetras;
    for (char letra = 'A'; letra <= 'Z'; ++letra) {
        todasLetras.push_back(letra);
    }

    // Seleccionar 8 letras aleatorias de todasLetras
    random_shuffle(todasLetras.begin(), todasLetras.end());
    vector<char> letrasSeleccionadas(todasLetras.begin(), todasLetras.begin() + 8);

    // Crear pares de las letras seleccionadas

    vector<char> paresLetras;
    for (char letra : letrasSeleccionadas) {
        paresLetras.push_back(letra);
        paresLetras.push_back(letra);
    }

    // Mezclar los pares de letras
    random_shuffle(paresLetras.begin(), paresLetras.end());

    // Rellenar el tablero con las letras mezcladas
    vector<vector<char>> tablero(4, vector<char>(4));
    int index = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            tablero[i][j] = paresLetras[index++];
        }
    }

    return tablero;
}

// Función para mostrar el tablero y devolver el tablero con celdas visibles y ocultas
vector<vector<char>> mostrarTablero(const vector<vector<char>>& tablero, const vector<vector<bool>>& descubiertas) {
    //cout << "  0 1 2 3\n";
    vector<vector<char>> tableroResultado(4, vector<char>(4, '*'));

    for (int i = 0; i < 4; ++i) {
        //cout << i << " ";
        for (int j = 0; j < 4; ++j) {
            if (descubiertas[i][j]) {
                //cout << tablero[i][j] << " ";
                tableroResultado[i][j] = tablero[i][j];
            } else {
                //cout << "* ";
            }
        }
        //cout << "\n";
    }

    return tableroResultado;
}
// Función para verificar si el tablero está completo es decir, con ella luego de completar la posición solicitada por el cliente tenemos que comprobar si la matriz esta con todos los valores descubiertos
bool tableroCompleto(const vector<vector<bool>>& descubiertas) {
    for (const auto& fila : descubiertas) {
        for (bool descubierta : fila) {
            if (!descubierta) {
                return false;
            }
        }
    }
    return true;
}

// Función para comprobar y actualizar el tablero de descubiertas
string comprobarYActualizarTablero(vector<vector<char>>& tablero, vector<vector<bool>>& descubiertas, int fila1, int col1, int fila2, int col2, int idJugador) {
    // Verificar si las posiciones están dentro de los límites del tablero
    if (fila1 < 0 || fila1 >= 4 || col1 < 0 || col1 >= 4 || fila2 < 0 || fila2 >= 4 || col2 < 0 || col2 >= 4)
        return "Posiciones fuera de los límites del tablero.";

    // Verificar si las letras en las dos posiciones coinciden
    if (tablero[fila1][col1] == tablero[fila2][col2] and descubiertas[fila1][col1] == false and descubiertas[fila2][col2] == false) {
        descubiertas[fila1][col1] = true;
        descubiertas[fila2][col2] = true;
        actualizarPuntuacion(idJugador, 1);
        return "¡Pareja encontrada!";
    } else
        if(descubiertas[fila1][col1] == true || descubiertas[fila2][col2] == true)
            return "Selecciono una o 2 posiciones ya descubiertas, aguarde su turno";
        return "Las casillas no coinciden.";
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
    cout << "  0 1 2 3\n";
    vector<vector<char>> tableroResultado(4, vector<char>(4, '*'));

    for (int i = 0; i < 4; ++i) {
        cout << i << " ";
        for (int j = 0; j < 4; ++j)
                cout << tablero[i][j] << " ";
        cout << "\n";
    }
}

void agregarJugador(int idJugador, int puntuacion) {
    Jugador nuevoJugador;
    nuevoJugador.idJugador = idJugador;
    nuevoJugador.puntuacion = puntuacion;
    clientesSockets.push_back(nuevoJugador);
}

void imprimirJugadores() {
    for (const auto& jugador : clientesSockets) {
        cout << "ID del Jugador: " << jugador.idJugador << ", Puntuación: " << jugador.puntuacion << endl;
    }
}

void eliminarJugador(int idJugador) {
    clientesSockets.erase(
        remove_if(clientesSockets.begin(), clientesSockets.end(),
                       [idJugador](const Jugador& jugador) {
                           return jugador.idJugador == idJugador;
                       }),
        clientesSockets.end());
}

Adivinanza deserializarAdivinanza(const vector<char> &buffer) {
    Adivinanza adivinanza;
    memcpy(&adivinanza, buffer.data(), sizeof(Adivinanza));
    return adivinanza;
}

vector<char> serializarConsulta(const Consulta &con) {
    vector<char> buffer;

    // Serializar el contenido del tablero (4x4)
    for (const auto &row : con.tableroActual) {
        buffer.insert(buffer.end(), row.begin(), row.end());
    }

    // Serializar la consulta
    if (con.consulta) {
        int consulta_len = strlen(con.consulta) + 1; // +1 para incluir el terminador nulo
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&consulta_len), reinterpret_cast<const char*>(&consulta_len) + sizeof(int));
        buffer.insert(buffer.end(), con.consulta, con.consulta + consulta_len);
    } else {
        int consulta_len = 0;
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&consulta_len), reinterpret_cast<const char*>(&consulta_len) + sizeof(int));
    }

    return buffer;
}

// vector<char> serializarConsulta(const Consulta& con) {
//     vector<char> buffer;
//     size_t tableroSize = con.tableroActual.size() * con.tableroActual[0].size();
//     buffer.resize(sizeof(size_t) + tableroSize + strlen(con.consulta) + 1);
//     size_t offset = 0;
//     size_t size = con.tableroActual.size();
//     memcpy(buffer.data(), &size, sizeof(size_t));
//     offset += sizeof(size_t);
//     for (const auto& fila : con.tableroActual) {
//         memcpy(buffer.data() + offset, fila.data(), fila.size());
//         offset += fila.size();
//     }
//     memcpy(buffer.data() + offset, con.consulta, strlen(con.consulta) + 1);
//     return buffer;
// }


vector<char> serializarRespuesta(const Respuesta &resp) {
    vector<char> buffer;

    // Serializar el contenido del tablero (4x4)
    for (const auto &row : resp.tableroActual) {
        buffer.insert(buffer.end(), row.begin(), row.end());
    }

    // Serializar la respuesta
    if (resp.respuesta) {
        int respuesta_len = strlen(resp.respuesta) + 1; // +1 para incluir el terminador nulo
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&respuesta_len), reinterpret_cast<const char*>(&respuesta_len) + sizeof(int));
        buffer.insert(buffer.end(), resp.respuesta, resp.respuesta + respuesta_len);
    } else {
        int respuesta_len = 0;
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&respuesta_len), reinterpret_cast<const char*>(&respuesta_len) + sizeof(int));
    }

    return buffer;
}

void enviarBuffer(int socket, const vector<char> &buffer) {
    int size = buffer.size();
    // Enviar el tamaño del buffer
    send(socket, &size, sizeof(int), 0);
    // Enviar el contenido del buffer
    send(socket, buffer.data(), size, 0);
}


vector<char> recibirBuffer(int socket) {
    int size;
    // Recibir el tamaño del buffer
    recv(socket, &size, sizeof(int), 0);
    // Reservar espacio para el contenido del buffer
    vector<char> buffer(size);
    // Recibir el contenido del buffer
    recv(socket, buffer.data(), size, 0);
    return buffer;
}

pair<int, list<int>> obtenerGanadores() {
    int puntajeMaximo = 0;
    list<int> ganadores;

    // Encontrar la puntuación máxima
    for (const auto& jugador : clientesSockets) {
        if (jugador.puntuacion > puntajeMaximo) {
            puntajeMaximo = jugador.puntuacion;
        }
    }

    // Encontrar todos los jugadores con la puntuación máxima
    for (const auto& jugador : clientesSockets) {
        if (jugador.puntuacion == puntajeMaximo) {
            ganadores.push_back(jugador.idJugador);
        }
    }

    return make_pair(puntajeMaximo, ganadores);
}