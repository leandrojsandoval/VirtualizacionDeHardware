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
#include <sstream>
#include <getopt.h>
#include <cstdlib> // Para usar atoi()
#include <csignal> // Para manejar señales
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ctime>
#include <cerrno>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <sys/param.h>

using namespace std;

#define BUFFER_SIZE 16384
#define MemPid "pidServidorSocket"
#define SERV_HOST_ADDR "127.0.0.1"     /* IP, only IPV4 support  */

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

sem_t* semaforos[1];
bool partida_activa = false;


/***********************************Semaforos**********************************/
void eliminar_Sem();
void inicializarSemaforos();
/***********************************Semaforos**********************************/

/***********************************Recursos**********************************/
void liberar_Recursos(int);

void mostrarAyuda();

/*******************Funciones particulares del caso***************************/
vector<vector<char>> generarTablero();
vector<vector<char>> mostrarTablero(const vector<vector<char>>& tablero, const vector<vector<bool>>& descubiertas);
bool tableroCompleto(const vector<vector<bool>>& descubiertas);
string comprobarYActualizarTablero(vector<vector<char>>& tablero, vector<vector<bool>>& descubiertas, int fila1, int col1, int fila2, int col2,int idJugador);
void actualizarPuntuacion(int idJugador, int puntos);

void mostrarTableroEnServidor(const vector<vector<char>>& tablero);
void agregarJugador(int idJugador, int puntuacion, char buffer[]);
void imprimirJugadores();
void eliminarCliente(int socket);

Adivinanza deserializarAdivinanza(const vector<char> &buffer);
vector<char> serializarConsulta(const Consulta &con);
vector<char> serializarRespuesta(const Respuesta &resp);
bool enviarBuffer(int socket, const vector<char> &buffer);
vector<char> recibirBuffer(int socket);

pair<int, list<Jugador>> obtenerGanadores();

void cerrarSocketsClientes();


int main(int argc, char *argv[]) {
    int opt;
    int port = -1;
    int max_clients = -1;
    
    struct option long_options[] = {
        {"puerto", required_argument, 0, 'p'},
        {"jugadores", required_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "p:j:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'j':
                max_clients = atoi(optarg);
                break;
            case 'h':
                if(argc != 2){
                    printf("Error en enviar parametros\n");
                    exit(EXIT_FAILURE);
                }
                else{
                    mostrarAyuda();
                    exit(EXIT_SUCCESS);
                }

            default:
                fprintf(stderr, "Opción no reconocida\n");
                exit(EXIT_FAILURE);
        }
    }

    if (port == -1 || max_clients == -1 || argc != 5) {
        fprintf(stderr, "Debe especificar el puerto y la cantidad de jugadores.\n");
        mostrarAyuda();
        exit(EXIT_FAILURE);
    }
    
    // Inicialización de semáforos
    inicializarSemaforos(); 

    // P(Servidor)
    sem_wait(semaforos[0]);

    signal(SIGUSR1, liberar_Recursos); // Manejar SIGUSR1 para liberar recursos
    //si esta la señal Ctrl+c la ignora.
    signal(SIGINT, SIG_IGN); // Ignorar SIGINT


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

    // Creación de memoria compartida
    //creamos una memoria compartida especial donde guardaremos el pid del servidorSocket y el socket de escucha.
    int idAux = shm_open(MemPid, O_CREAT | O_RDWR, 0600);
    if(idAux == -1){
        cerr << "Error al crear la memoria compartida: " << strerror(errno) << endl;
        close(server_fd);
        eliminar_Sem();
        exit(EXIT_FAILURE);
    }

    if (ftruncate(idAux, sizeof(dato)) == -1) {
        cerr << "Error al configurar el tamaño de la memoria compartida: " << strerror(errno) << endl;
        close(idAux); // Cerrar idAux antes de salir
        close(server_fd);
        shm_unlink(MemPid);
        eliminar_Sem();
        exit(EXIT_FAILURE);
    }

    dato *pidA = (dato*)mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, idAux, 0);
    if (pidA == MAP_FAILED) {
        cerr << "Error al mapear la memoria compartida: " << strerror(errno) << endl;
        close(idAux); // Cerrar idAux antes de salir
        close(server_fd);
        shm_unlink(MemPid);
        eliminar_Sem();
        exit(EXIT_FAILURE);
    }
    // Cerrar idAux después de mapear
    close(idAux);

    // Asignar valores a la estructura en la memoria compartida
    pidA->pidServ = getpid();
    pidA->Socket_Escucha = server_fd;
    if (munmap(pidA, sizeof(dato)) == -1) {
        cerr << "Error al desmapear la memoria compartida: " << strerror(errno) << endl;
        close(server_fd); // Asegurarse de que server_fd se cierra en caso de error
        shm_unlink(MemPid);
        eliminar_Sem();
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %d\n", port);

    // Aceptar conexiones hasta el límite de max_clients
    int connected_clients = 0;
    while (connected_clients < max_clients) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            close(server_fd);
            liberar_Recursos(1);
            exit(EXIT_FAILURE);
        }

        //recibiendo nickname por parte del cliente
        char bufferConexion[1024] = {0};
        int valread = recv(new_socket, bufferConexion, 1024, 0);
        if (valread < 0) {
            perror("Error en recv");
            close(server_fd);
            liberar_Recursos(1);
            exit(EXIT_FAILURE);
        }
        if (valread == 0){
            printf("\n Error de conexión: cliente desconectado.\n");
            continue;
        }

        agregarJugador(new_socket,0,bufferConexion);

        char registro[1024];
        // Convertimos el número a cadena
        sprintf(registro, "%d", new_socket);

        // Concatenamos la cadena del número al buffer original
        strcat(bufferConexion, " ");
        strcat(bufferConexion, registro);

        char leyenda[2048] = "Nuevo jugador conectado cuyo nickname y puerto son: ";
        strcat(leyenda, bufferConexion);

        int send_result = send(new_socket, leyenda, strlen(leyenda), 0);
        if (send_result < 0) {
            perror("Error en send en armado de conexión con cliente");
            close(new_socket);
            cerrarSocketsClientes();
            exit(EXIT_FAILURE);
        }
        connected_clients++;
    }
    //generar tablero de partida
    srand(time(0)); // Semilla para el generador de números aleatorios
    vector<vector<char>> tablero_partida = generarTablero();
    //eliminar esto una ves que el servidor pase a ejecutar en segundo plano...
    mostrarTableroEnServidor(tablero_partida);
    //generar vector de descubiertos
    vector<vector<bool>> descubiertas(4, vector<bool>(4, false));

    // Notificar a cada cliente que el juego ha comenzado
    for (auto it = clientesSockets.begin(); it != clientesSockets.end(); ) {
        const char *message = "El juego ha comenzado\n";
        printf("Notificando al cliente %d\n", it->idJugador);
        int noti = send(it->idJugador, message, strlen(message), 0);

        if (noti < 0) {
            perror("Error al enviar mensaje");
            printf("Cliente con socket %d eliminado de la lista debido a error de envío.\n", it->idJugador);
            close(it->idJugador);
            it = clientesSockets.erase(it);
        } else if (noti == 0) {
            printf("Cliente con socket %d desconectado.\n", it->idJugador);
            close(it->idJugador);
            it = clientesSockets.erase(it);
        } else {
            ++it;
        }
    }

    bool respMensaje = false;
    partida_activa = true;
    while (!tableroCompleto(descubiertas) && clientesSockets.size() > 1) {
        auto jugador = clientesSockets.begin();
        while (jugador != clientesSockets.end() && !tableroCompleto(descubiertas) && clientesSockets.size() > 1) {
            if (!tableroCompleto(descubiertas) && clientesSockets.size() > 1) {
                char menIni[1024] = {0};
                printf("enviando turno a: %d\n", jugador->idJugador);

                // Intentar recibir para verificar la conexión
                char bufferTest[1];
                int recvResult = recv(jugador->idJugador, bufferTest, sizeof(bufferTest), MSG_PEEK | MSG_DONTWAIT);
                if (recvResult == 0 || (recvResult < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                    perror("Jugador desconectado detectado por recv");
                    close(jugador->idJugador);
                    jugador = clientesSockets.erase(jugador);
                    continue;
                }
                strcpy(menIni, "Siguiente turno!");
                if (send(jugador->idJugador, menIni, strlen(menIni), 0) <= 0) {
                    perror("Error en mensaje o jugador desconectado");
                    printf("Error, jugador desconectado o error de conexion\n");
                    close(jugador->idJugador);
                    jugador = clientesSockets.erase(jugador);
                    continue;
                }
                printf("turno enviado\n");
                // Generar consulta actual para enviar al cliente.
                Consulta con;
                con.tableroActual = mostrarTablero(tablero_partida, descubiertas);
                const char *consultaStr = "Es tu turno! ¿Dónde se encuentran coincidencias?\n";
                con.consulta = new char[strlen(consultaStr) + 1];
                strcpy(con.consulta, consultaStr); 

                // Serializar consulta para enviárselo al cliente
                vector<char> consulta_serializada = serializarConsulta(con);

                // Enviar consulta al cliente
                respMensaje = enviarBuffer(jugador->idJugador, consulta_serializada);
                if (!respMensaje) {
                    printf("Error, jugador desconectado o error de conexion\n");
                    close(jugador->idJugador);
                    jugador = clientesSockets.erase(jugador);
                    delete[] con.consulta;
                    continue;
                }

                // Recibir adivinanza del cliente y deserializarla
                vector<char> adivinanza_recibida = recibirBuffer(jugador->idJugador);
                if (adivinanza_recibida.empty()) {
                    printf("Error, jugador desconectado o error de conexion\n");
                    close(jugador->idJugador);
                    jugador = clientesSockets.erase(jugador);
                    delete[] con.consulta;
                    continue;
                }

                Adivinanza adivinanza = deserializarAdivinanza(adivinanza_recibida);

                // Validar respuesta del cliente
                string resultado = comprobarYActualizarTablero(tablero_partida, descubiertas, adivinanza.filaLetra1, adivinanza.columnaLetra1, adivinanza.filaLetra2, adivinanza.columnaLetra2, jugador->idJugador);

                // Generar tablero actual en base al procesamiento
                Respuesta resp;
                resp.tableroActual = mostrarTablero(tablero_partida, descubiertas);
                resp.respuesta = new char[resultado.length() + 1];
                strcpy(resp.respuesta, resultado.c_str());

                // Serializar respuesta y enviar respuesta al jugador
                vector<char> respuesta_serializada = serializarRespuesta(resp);
                respMensaje = enviarBuffer(jugador->idJugador, respuesta_serializada);
                if (!respMensaje) {
                    printf("Error, jugador desconectado o error de conexion\n");
                    close(jugador->idJugador);
                    jugador = clientesSockets.erase(jugador);
                    delete[] con.consulta;
                    delete[] resp.respuesta;
                    continue;
                }

                // Limpiar memoria
                delete[] con.consulta;
                delete[] resp.respuesta;
                ++jugador;
            } else {
                break;
            }
        }
    }

    printf("Notificando finalización de juego\n");

    if (clientesSockets.size() <= 1) {
        printf("\n¡El juego ha terminado!: no hay suficientes jugadores para continuar.\n");
        for (const auto& jugador : clientesSockets) {
            const char* message = "¡El juego ha terminado!: no hay suficientes jugadores para continuar.\n";
            send(jugador.idJugador, message, strlen(message), 0);
        }
    } else {
        printf("\n¡El juego ha terminado!\n");

        // Obtener los ganadores y su puntaje
        pair<int, list<Jugador>> resultado = obtenerGanadores();
        int maxPuntuacion = resultado.first;
        list<Jugador> ganadores = resultado.second;

        // Construir mensaje de finalización del juego con ganadores
        ostringstream mensajeFinal;
        mensajeFinal << "¡El juego ha terminado!\n";
        mensajeFinal << "Puntuación máxima: " << maxPuntuacion << "\n";
        mensajeFinal << "Ganadores:\n";
        for (const auto& ganador : ganadores) {
            mensajeFinal << "ID Jugador: " << ganador.idJugador << ", Nickname: " << ganador.nickname << ", Puntuación: " << ganador.puntuacion << "\n";
            printf("Ganador %d, %s\n",ganador.idJugador,ganador.nickname);
        }
        cout << mensajeFinal.str();

        string mensajeFinalStr = mensajeFinal.str();
        const char* mensajeFinalCStr = mensajeFinalStr.c_str();

        printf("Mensaje final: %s\n",mensajeFinalCStr);

        for (const auto& jugador : clientesSockets) {
            int bytesEnviadosUltimoMensaje = send(jugador.idJugador, mensajeFinalCStr, strlen(mensajeFinalCStr), 0);
            if (bytesEnviadosUltimoMensaje == -1) {
                cerr << "Error al enviar mensaje a ID Jugador: " << jugador.idJugador << endl;
            } else if (bytesEnviadosUltimoMensaje < strlen(mensajeFinalCStr)) {
                // Manejo de mensaje parcialmente enviado
                cerr << "Mensaje parcialmente enviado a ID Jugador: " << jugador.idJugador << endl;
            }
        }
    }

    partida_activa = false;

    liberar_Recursos(1);

    // Cerrar el socket del servidor
    close(server_fd);

    exit(EXIT_SUCCESS);
}

void eliminar_Sem() {
    if (sem_close(semaforos[0]) == -1) {
        cerr << "Error al cerrar el semáforo: " << strerror(errno) << endl;
    }
    if(sem_unlink("servidorSocket") == -1) {
        cerr << "Error al eliminar el semáforo: " << strerror(errno) << endl;
    }
}

void inicializarSemaforos() {
    semaforos[0] = sem_open("servidorSocket", O_CREAT, 0600, 1);
    
    // Si dicho semáforo vale 0 en ese momento significa que ya hay otra instancia de semáforo ejecutando por lo que cerramos el proceso.
    // porque carajo mandé el 85, no recuerdo...
    int valorSemServi = 85;
    sem_getvalue(semaforos[0], &valorSemServi);
    if (valorSemServi == 0)
        exit(EXIT_FAILURE);
}

void liberar_Recursos(int signum) {
    if(partida_activa)
        return;
    eliminar_Sem();

    // Creamos una memoria compartida especial donde guardaremos el pid del servidorSocket y el socket de escucha.
    int idAux = shm_open(MemPid, O_CREAT | O_RDWR, 0600);
    if (idAux == -1) {
        perror("Error en shm_open");
        cerrarSocketsClientes();
        exit(EXIT_FAILURE);
    }

    dato *pidA = (dato*)mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, idAux, 0);
    if (pidA == MAP_FAILED) {
        perror("Error en mmap");
        close(idAux);
        cerrarSocketsClientes();
        exit(EXIT_FAILURE);
    }

    close(idAux);

    // Añadir verificación y mensajes de depuración
    if (pidA->Socket_Escucha > 0) { // Verificar si el socket es válido
        if (shutdown(pidA->Socket_Escucha, SHUT_RDWR) == -1) {
            perror("Error en shutdown");
        }
    } else {
        fprintf(stderr, "Socket_Escucha no es válido: %d\n", pidA->Socket_Escucha);
    }

    if (munmap(pidA, sizeof(dato)) == -1) {
        perror("Error en munmap");
    }

    if (shm_unlink(MemPid) == -1) {
        perror("Error en shm_unlink");
    }

    cerrarSocketsClientes();
    printf("Recursos liberados.\n");
    exit(EXIT_SUCCESS);
}


void mostrarAyuda() {
    printf("Uso: ./Servidor -p puerto -j cantJugadores\n");
    printf("     ./Servidor --puerto puerto --jugadores cantJugadores\n");
    printf("     ./Servidor -h\n");
    printf("     ./Servidor --help\n");
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

void agregarJugador(int idJugador, int puntuacion,char buffer[]) {
    Jugador nuevoJugador;
    nuevoJugador.idJugador = idJugador;
    nuevoJugador.puntuacion = puntuacion;
    strncpy(nuevoJugador.nickname, buffer, sizeof(nuevoJugador.nickname) - 1);
    nuevoJugador.nickname[sizeof(nuevoJugador.nickname) - 1] = '\0';  // Asegurar la terminación en nulo

    clientesSockets.push_back(nuevoJugador);
    printf("nuevo jugador agregado con nickname: %s y id %d\n",nuevoJugador.nickname,idJugador);
}

void imprimirJugadores() {
    for (const auto& jugador : clientesSockets) {
        cout << "ID del Jugador: " << jugador.idJugador << ", Puntuación: " << jugador.puntuacion << endl;
    }
}

void eliminarCliente(int socket) {
    for (auto it = clientesSockets.begin(); it != clientesSockets.end(); ++it) {
        if (it->idJugador == socket) {
            clientesSockets.erase(it);
            cout << "Cliente con socket " << socket << " eliminado de la lista." << endl;
            break;
        }
    }
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

bool enviarBuffer(int socket, const vector<char> &buffer) {
    int size = buffer.size();
    // Enviar el tamaño del buffer
    int bytesSent = send(socket, &size, sizeof(int), 0);
    if (bytesSent <= 0) {
        if (bytesSent == 0) {
            // El cliente se desconectó
            cerr << "El cliente se ha desconectado." << endl;
        } else {
            // Error en el envío
            perror("Error en send");
        }
        return false;
    }
    // Enviar el contenido del buffer
    bytesSent = send(socket, buffer.data(), size, 0);
    if (bytesSent <= 0) {
        if (bytesSent == 0) {
            // El cliente se desconectó
            cerr << "El cliente se ha desconectado." << endl;
        } else {
            // Error en el envío
            perror("Error en send");
        }
        return false;
    }
    return true;
}

vector<char> recibirBuffer(int socket) {
    int size;
    // Recibir el tamaño del buffer
    int bytesReceived = recv(socket, &size, sizeof(int), 0);
    if (bytesReceived <= 0) {
        if (bytesReceived == 0) {
            // El cliente se desconectó
            cerr << "El cliente se ha desconectado." << endl;
        } else {
            // Error en la recepción
            perror("Error en recv");
        }
        return {};
    }
    // Reservar espacio para el contenido del buffer
    vector<char> buffer(size);
    // Recibir el contenido del buffer
    bytesReceived = recv(socket, buffer.data(), size, 0);
    if (bytesReceived <= 0) {
        if (bytesReceived == 0) {
            // El cliente se desconectó
            cerr << "El cliente se ha desconectado." << endl;
        } else {
            // Error en la recepción
            perror("Error en recv");
        }
        return {};
    }
    return buffer;
}

pair<int, list<Jugador>> obtenerGanadores() {
    int puntajeMaximo = 0;
    list<Jugador> ganadores;

    // Encontrar la puntuación máxima
    for (const auto& jugador : clientesSockets) {
        if (jugador.puntuacion > puntajeMaximo) {
            puntajeMaximo = jugador.puntuacion;
        }
    }

    // Encontrar todos los jugadores con la puntuación máxima
    for (const auto& jugador : clientesSockets) {
        if (jugador.puntuacion == puntajeMaximo) {
            ganadores.push_back(jugador);
        }
    }

    return make_pair(puntajeMaximo, ganadores);
}

void cerrarSocketsClientes() {
    for (auto it = clientesSockets.begin(); it != clientesSockets.end();) {
        close(it->idJugador);
        it = clientesSockets.erase(it);
    }
}