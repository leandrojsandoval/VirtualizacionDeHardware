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
#include <ctime>
#include <cstdlib>
#include <csignal>
#include <atomic>
#include <sys/param.h>

using namespace std;

typedef struct {
    int pidServ;
    int Socket_Escucha;
} dato;

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

typedef struct {
    int puntuacion;
} Jugador;

// Variable global para indicar que el servidor debe detenerse
atomic<bool> serverRunning(true);

vector<Jugador> jugadores;
vector<int> clientesSockets;
vector<int> nuevosClientesSockets;

sem_t* semaforos[1];

#define MemPid "pidServidorSocket"
#define SERV_HOST_ADDR "127.0.0.1"     /* IP, only IPV4 support  */

/***********************************Semaforos**********************************/
void eliminar_Sem();
void inicializarSemaforos();
/***********************************Semaforos**********************************/

/***********************************Recursos**********************************/
void liberar_Recursos(int);
/***********************************Recursos**********************************/

/*******************Funciones particulares del caso***************************/
vector<vector<char>> generarTablero();
vector<vector<char>> mostrarTablero(const vector<vector<char>>& tablero, const vector<vector<bool>>& descubiertas);
bool tableroCompleto(const vector<vector<bool>>& descubiertas);
string comprobarYActualizarTablero(vector<vector<char>>& tablero, vector<vector<bool>>& descubiertas, int fila1, int col1, int fila2, int col2);
vector<char> serializarRespuesta(const Respuesta& resp);


bool Ayuda(const char *cad);

//atender señales
void signalHandler(int signum) {
    std::cout << "Interrupción recibida: " << signum << std::endl;
    serverRunning = false;  // Indicar que el servidor debe detenerse
    // Informa a todos los clientes que el servidor se ha caído o detenido
    string mensajeFinal = "El servidor ha caído o ha sido detenido";
    for (int clienteSocket : clientesSockets) {
        write(clienteSocket, mensajeFinal.c_str(), mensajeFinal.length());  // Envía el mensaje a cada cliente
        close(clienteSocket);  // Cierra la conexión con el cliente
    }
    liberar_Recursos(signum);
}


/**inputs puerto servidor, cantidad de clientes posibles**/
int main(int argc, char *argv[]){

    int cantidadJugadores = -1;
    int puerto = -1;
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        Ayuda(argv[1]);
        exit(EXIT_SUCCESS);
    } else if (argc != 5) {
        cout << "Uso incorrecto. Para ayuda ejecute ./Servidor -h o ./Servidor --help" << endl;
        exit(EXIT_FAILURE);
    }

    // Obtener cantidad de jugadores y puerto
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--jugadores") == 0) && i + 1 < argc) {
            cantidadJugadores = atoi(argv[i + 1]);
            i++;
        } else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--puerto") == 0) && i + 1 < argc) {
            puerto = atoi(argv[i + 1]);
            i++;
        } else {
            cout << "Uso incorrecto. Para ayuda ejecute ./Servidor -h o ./Servidor --help" << endl;
            exit(EXIT_FAILURE);
        }
    }

    if (cantidadJugadores <= 0 || puerto <= 0) {
        cout << "Error: número de jugadores y puerto deben ser mayores a 0." << endl;
        exit(EXIT_FAILURE);
    }


    pid_t pid, sid; //Proceso y sesión
    int i;

    // Ignora la señal de E / S del terminal, señal de PARADA
    signal(SIGTTOU, SIG_IGN); // Ignorar señal de salida del terminal
    signal(SIGTTIN, SIG_IGN); // Ignorar señal de entrada del terminal
    signal(SIGTSTP, SIG_IGN); // Ignorar señal de parada
    signal(SIGHUP, SIG_IGN);  // Ignorar señal de colgar

    pid = fork(); // Crear un nuevo proceso
    if (pid < 0) 
        exit(EXIT_FAILURE); // Finaliza el proceso padre, haciendo que el proceso hijo sea un proceso en segundo plano, es decir, el fork falló
    if (pid > 0)
        exit(EXIT_SUCCESS); // Salir del proceso padre
    
    // Cree un nuevo grupo de procesos, en este nuevo grupo de procesos, el proceso secundario se convierte en el primer proceso de este grupo de procesos, de modo que el proceso se separa de todos los terminales
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE); // Si no se puede crear la sesión, salir
    }

    // Cree un nuevo proceso hijo nuevamente, salga del proceso padre, asegúrese de que el proceso no sea el líder del proceso y haga que el proceso no pueda abrir una nueva terminal
    pid = fork();
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    else if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    // Cierre todos los descriptores de archivos heredados del proceso padre que ya no son necesarios
    for (i = 0; i < NOFILE; close(i++));

    // Establece la palabra de protección del archivo en 0 en el momento
    umask(0); // Restablecer la máscara de creación de archivos

    // Inicialización de semáforos
    inicializarSemaforos(); 

    // P(Servidor)
    sem_wait(semaforos[0]);

    // Manejo de señales, cuando se reciban algunas de las dos señales se ejecutará su correspondiente función.
    signal(SIGUSR1, liberar_Recursos); // Manejar SIGUSR1 para liberar recursos
    //si esta la señal Ctrl+c la ignora.
    //signal(SIGINT, SIG_IGN); // Ignorar SIGINT

    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    struct sockaddr_in serverConfig; // Estructura de configuración del servidor
    memset(&serverConfig, '0', sizeof(serverConfig)); // Inicializar estructura a cero

    // Familia de direcciones IPv4
    serverConfig.sin_family = AF_INET; //IPV4: 127.0.0.1

    //en caso de pasar la ip del servidor descomentar esto.
    //serverConfig.sin_addr.s_addr = inet_addr(argv[1]);// Configurar dirección IP del servidor
    serverConfig.sin_addr.s_addr = INADDR_ANY;// Configurar dirección IP del servidor

    
    //en caso de pasar la ip del servidor descomentar esto.
    serverConfig.sin_port = htons(puerto);

    int socketEscucha = socket(AF_INET, SOCK_STREAM, 0);
    if (socketEscucha == -1) {
        cerr << "Error al crear el socket: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    //nos va a linkear/relacionar nuestro socket con nuestra configuración.
    //bind asinga una direccion ip y un puerto al socket.
    if (bind(socketEscucha, (struct sockaddr *)&serverConfig, sizeof(serverConfig)) < 0) {
        cerr << "Error al enlazar el socket: " << strerror(errno) << endl;
        close(socketEscucha);
        exit(EXIT_FAILURE);
    }

    // Configurar socket en modo escucha
    if (listen(socketEscucha, cantidadJugadores) < 0) {
        cerr << "Error al configurar el socket en modo escucha: " << strerror(errno) << endl;
        close(socketEscucha);
        exit(EXIT_FAILURE);
    }

    // Creación de memoria compartida
    //creamos una memoria compartida especial donde guardaremos el pid del servidorSocket y el socket de escucha.
    int idAux = shm_open(MemPid, O_CREAT | O_RDWR, 0600);
    if(idAux == -1){
        cerr << "Error al crear la memoria compartida: " << strerror(errno) << endl;
        close(socketEscucha);
        exit(EXIT_FAILURE);
    }

    if (ftruncate(idAux, sizeof(dato)) == -1) {
        cerr << "Error al configurar el tamaño de la memoria compartida: " << strerror(errno) << endl;
        close(socketEscucha);
        shm_unlink(MemPid);
        exit(EXIT_FAILURE);
    }

    dato *pidA = (dato*)mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, idAux, 0);
    if (pidA == MAP_FAILED) {
        cerr << "Error al mapear la memoria compartida: " << strerror(errno) << endl;
        close(socketEscucha);
        shm_unlink(MemPid);
        exit(EXIT_FAILURE);
    }
    close(idAux);
    pidA->pidServ = getpid();
    pidA->Socket_Escucha = socketEscucha;
    munmap(pidA, sizeof(dato));


    srand(time(0)); // Semilla para el generador de números aleatorios

    //Creamos el tablero de juego
    vector<vector<char>> tablero = generarTablero();
    vector<vector<bool>> descubiertas(4, vector<bool>(4, false));
    bool juegoterminado = false;
    int turnoActual = 0;

    // Esperar hasta que se conecten los jugadores mínimos requeridos
    while (serverRunning && clientesSockets.size() < cantidadJugadores) {
        struct sockaddr_in clientConfig;
        socklen_t clientConfigLen = sizeof(clientConfig);
        int nuevoSocket = accept(socketEscucha, (struct sockaddr *)&clientConfig, &clientConfigLen);
        if (nuevoSocket >= 0) {
            clientesSockets.push_back(nuevoSocket);
            jugadores.push_back(Jugador{0});
            cout << "Nuevo jugador conectado. Total jugadores: " << clientesSockets.size() << endl;
        }
    }

    while (serverRunning && !juegoterminado) {
        int jugadorIndex = turnoActual % clientesSockets.size();
        int clienteSocket = clientesSockets[jugadorIndex];

        // Recibir datos del cliente
        Adivinanza jugada;
        int bytesRecibidos = recv(clienteSocket, &jugada, sizeof(Adivinanza), 0);
        if (bytesRecibidos < 0) {
            cerr << "Error al recibir datos del cliente: " << strerror(errno) << endl;
            continue;
        }

        // Procesar la jugada del cliente
        string resultado = comprobarYActualizarTablero(tablero, descubiertas, jugada.filaLetra1, jugada.columnaLetra1, jugada.filaLetra2, jugada.columnaLetra2);

        // Actualizar puntuación del jugador
        jugadores[jugadorIndex].puntuacion += jugada.puntuacion;

        // Preparar la respuesta
        Respuesta respuesta = {resultado, jugadores[jugadorIndex].puntuacion, mostrarTablero(tablero, descubiertas)};

        // Enviar la respuesta al cliente
        vector<char> datosSerializados = serializarRespuesta(respuesta);
        send(clienteSocket, datosSerializados.data(), datosSerializados.size(), 0);

        // Verificar si el juego está completo
        juegoterminado = tableroCompleto(descubiertas);

        // Pasar al siguiente jugador
        turnoActual++;
    }

    // Determinar el ganador
    Jugador ganador = jugadores[0];
    for (const auto &jugador : jugadores) {
        if (jugador.puntuacion > ganador.puntuacion) {
            ganador = jugador;  // Actualiza el ganador si encuentra un jugador con mayor puntuación
        }
    }

    // Informa a todos los clientes que el juego ha terminado y quién es el ganador
    string mensajeFinal = "El juego ha terminado. El ganador es el jugador con " + to_string(ganador.puntuacion) + " puntos.";
    for (int clienteSocket : clientesSockets) {
        write(clienteSocket, mensajeFinal.c_str(), mensajeFinal.length());  // Envía el mensaje a cada cliente
        close(clienteSocket);  // Cierra la conexión con el cliente
    }
    close(socketEscucha);
    cout << mensajeFinal << endl;  // Imprime el mensaje final en el servidor
    liberar_Recursos(1);
    exit(EXIT_SUCCESS);
}



void eliminar_Sem() {
    sem_close(semaforos[0]);
    sem_unlink("servidorSocket");
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
    eliminar_Sem();
    //creamos una memoria compartida especial donde guardaremos el pid del servidorSocket y el socket de escucha.
    int idAux = shm_open(MemPid, O_CREAT | O_RDWR, 0600);
    dato *pidA = (dato*)mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, idAux, 0);
    close(idAux);
    shutdown(pidA->Socket_Escucha, SHUT_RDWR);
    munmap(pidA, sizeof(dato));
    shm_unlink(MemPid);
    exit(EXIT_SUCCESS);
}

bool Ayuda(const char *cad) {
    if (cad == NULL)
        return false;

    printf("uso: %s \n", cad);
    printf("Este programa es un servidor que permite a los clientes conectarse y jugar un juego de parejas.\n");
    printf("Parámetros:\n");
    printf("-h, --help\t\tMuestra esta ayuda.\n");
    printf("-j, --jugadores\t\tNúmero de jugadores.\n");
    printf("-p, --puerto\t\tNúmero de puerto para la conexión.\n");
    
    return true;
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
    cout << "  0 1 2 3\n";
    vector<vector<char>> tableroResultado(4, vector<char>(4, '*'));

    for (int i = 0; i < 4; ++i) {
        cout << i << " ";
        for (int j = 0; j < 4; ++j) {
            if (descubiertas[i][j]) {
                cout << tablero[i][j] << " ";
                tableroResultado[i][j] = tablero[i][j];
            } else {
                cout << "* ";
            }
        }
        cout << "\n";
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
string comprobarYActualizarTablero(vector<vector<char>>& tablero, vector<vector<bool>>& descubiertas, int fila1, int col1, int fila2, int col2) {
    // Verificar si las posiciones están dentro de los límites del tablero
    if (fila1 < 0 || fila1 >= 4 || col1 < 0 || col1 >= 4 || fila2 < 0 || fila2 >= 4 || col2 < 0 || col2 >= 4)
        return "Posiciones fuera de los límites del tablero.";

    // Verificar si las letras en las dos posiciones coinciden
    if (tablero[fila1][col1] == tablero[fila2][col2]) {
        descubiertas[fila1][col1] = true;
        descubiertas[fila2][col2] = true;
        return "¡Pareja encontrada!";
    } else
        return "Las casillas no coinciden.";
}

// Función para serializar la estructura Respuesta
vector<char> serializarRespuesta(const Respuesta& resp) {
    vector<char> data;

    // Serializar la cadena 'respuesta'
    int respuestaLength = resp.respuesta.length();
    data.insert(data.end(), reinterpret_cast<const char*>(&respuestaLength), reinterpret_cast<const char*>(&respuestaLength) + sizeof(respuestaLength));
    data.insert(data.end(), resp.respuesta.begin(), resp.respuesta.end());

    // Serializar 'puntuacion'
    data.insert(data.end(), reinterpret_cast<const char*>(&resp.puntuacion), reinterpret_cast<const char*>(&resp.puntuacion) + sizeof(resp.puntuacion));

    // Serializar 'tableroActual'
    for (const auto& fila : resp.tableroActual) {
        for (char c : fila) {
            data.push_back(c);
        }
    }

    return data;
}

