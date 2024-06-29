#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <iostream>
#include <getopt.h>
#include <cstdlib> // Para usar atoi()
#include <iostream>
#include <csignal> // Para manejar señales
#include <netdb.h> // Para gethostbyname

using namespace std;

#define BUFFER_SIZE 16384

typedef struct {
    int filaLetra1;
    int columnaLetra1;
    int filaLetra2;
    int columnaLetra2;
} Adivinanza;

typedef struct {
    vector<vector<char>> tableroActual;
    char *consulta;
} Consulta;

typedef struct {
    vector<vector<char>> tableroActual;
    char *respuesta;
} Respuesta;

void mostrarAyuda();

vector<char> recibirBuffer(int socket);
bool enviarBuffer(int socket, const vector<char> &buffer);
Adivinanza deserializarAdivinanza(const vector<char> &buffer);
Consulta deserializarConsulta(const vector<char> &buffer);
vector<char> serializarAdivinanza(const Adivinanza &adv);
vector<char> serializarConsulta(const Consulta &con);
Respuesta deserializarRespuesta(const vector<char> &buffer);

void mostrarTableroEnCliente(const vector<vector<char>>& tablero);

int sock = 0;

int main(int argc, char *argv[]) {
    int opt;
    int port = -1;
    char nickname[50] = {0};
    char server_ip[50] = {0};

    struct option long_options[] = {
        {"nickname", required_argument, 0, 'n'},
        {"puerto", required_argument, 0, 'p'},
        {"servidor", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "n:p:s:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'n':
                strncpy(nickname, optarg, sizeof(nickname)-1);
                nickname[sizeof(nickname) - 1] = '\0';  // Asegurar la terminación en nulo
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 's':
                strncpy(server_ip, optarg, sizeof(server_ip)-1);
                server_ip[sizeof(server_ip) - 1] = '\0';  // Asegurar la terminación en nulo
                break;
            case 'h':
                mostrarAyuda();
                exit(EXIT_SUCCESS);
            default:
                fprintf(stderr, "Opción no reconocida\n");
                exit(EXIT_FAILURE);
        }
    }

    if (port == -1 || strlen(nickname) == 0 || strlen(server_ip) == 0) {
        fprintf(stderr, "Debe especificar el nickname, el puerto y el servidor.\n");
        mostrarAyuda();
        exit(EXIT_FAILURE);
    }

    // Verificar que el nickname no supere la longitud máxima permitida
    if (strlen(nickname) >= 50) {
        fprintf(stderr, "El nickname es demasiado largo. Debe tener menos de 50 caracteres.\n");
        exit(EXIT_FAILURE);
    }

    //si esta la señal Ctrl+c la ignora.
    signal(SIGINT, SIG_IGN); // Ignorar SIGINT

    int valread;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    vector<char> buffer(BUFFER_SIZE);

    // Crear el socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error al crear el socket \n");
        return -1;
    }

    // Intentar obtener la dirección IP usando gethostbyname
    server = gethostbyname(server_ip);
    if (server == NULL) {
        fprintf(stderr, "Error, no hay tal host\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);


    // Convertir direcciones IPv4 e IPv6 de texto a binario
/*
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\n Dirección no soportada \n");
        return -1;
    }
*/
    // Conectarse al servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Error de conexión con el servidor\n");
        return -1;
    }

    printf("Conectado al servidor\n");

    if (send(sock, nickname, strlen(nickname), 0) < 0) {
        perror("Error el servidor puede estar desconectado");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Nickname enviado al servidor: %s\n", nickname);

    char confirmacion[2048] = {0};
    // Esperando el mensaje de confirmación del servidor en la cual recibo mi id y nickname.
    valread = recv(sock, confirmacion, sizeof(confirmacion) - 1, 0);
    if (valread < 0) {
        perror("Error en recv");
        close(sock);
        exit(EXIT_FAILURE);
    }
    if(valread == 0){
        perror("Servidor desconectado.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Aseguramos que el buffer de confirmación termine en '\0'
    confirmacion[valread] = '\0';
    printf("Mensaje de confirmación del servidor: %s\n", confirmacion);

    // Esperar el mensaje de inicio del juego
    valread = recv(sock, buffer.data(), BUFFER_SIZE, 0);
    if (valread < 0) {
        perror("Error al recibir datos del servidor \n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    if(valread == 0){
        perror("Servidor desconectado.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("%s\n", buffer.data());

    // Comunicación con el servidor
    bool juegoTerminado=false;
    while (!juegoTerminado) {
        // Esperar hasta que el servidor indique que es el turno del cliente
        char menIni[1024] = {0};
        valread = recv(sock, menIni, sizeof(menIni) - 1, 0);
        if (valread < 0) {
            perror("Error al recibir datos del servidor o conexión cerrada \n");
            close(sock);
            exit(EXIT_FAILURE);
        }
        if(valread == 0){
            perror("Servidor desconectado.\n");
            close(sock);
            exit(EXIT_FAILURE);
        }

        if (strncmp(menIni, "¡El juego ha terminado!", strlen("¡El juego ha terminado!")) == 0) {
            printf("%s", menIni); // Imprimir el mensaje de juego terminado
            juegoTerminado = true;
            break;
        }

        vector<char> consulta_serializada = recibirBuffer(sock);
        if (consulta_serializada.empty()) {
            printf("Error, Servidor desconectado o error de conexion\n");
            close(sock);
            exit(EXIT_FAILURE);
        }

        Consulta consulta = deserializarConsulta(consulta_serializada);

        printf("%s", consulta.consulta);
        cout << "  0 1 2 3\n";
        int cont=0;
        for (const auto &fila : consulta.tableroActual) {
            printf("%d ",cont);
            for (char letra : fila) {
                printf("%c ", letra);
            }
            printf("\n");
            cont++;
        }
        cont=0;

        // Leer la adivinanza del usuario
        Adivinanza adv;
        //Para los do while tener en cuenta que estos cierren cuando el servidor se desconecte. es decir, enviar una condición extra
        do {
            printf("Ingrese la fila y columna de la primera letra (0-3): ");
            scanf("%d %d", &adv.filaLetra1, &adv.columnaLetra1);
        } while (adv.filaLetra1 < 0 || adv.filaLetra1 > 3 || adv.columnaLetra1 < 0 || adv.columnaLetra1 > 3);

        do {
            printf("Ingrese la fila y columna de la segunda letra (0-3): ");
            scanf("%d %d", &adv.filaLetra2, &adv.columnaLetra2);
        } while (adv.filaLetra2 < 0 || adv.filaLetra2 > 3 || adv.columnaLetra2 < 0 || adv.columnaLetra2 > 3 || (adv.filaLetra2 == adv.filaLetra1 && adv.columnaLetra2 == adv.columnaLetra1));

        // Serializar la adivinanza y enviarla al servidor
        vector<char> adivinanza_serializada = serializarAdivinanza(adv);
        if(!enviarBuffer(sock, adivinanza_serializada)){
            printf("Error, Servidor desconectado o error de conexion\n");
            close(sock);
            exit(EXIT_FAILURE);
        }

        // Esperar la respuesta del servidor
        vector<char> respuesta_serializada = recibirBuffer(sock);
        if (respuesta_serializada.empty()) {
            printf("Error, Servidor desconectado o error de conexion\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
        
        Respuesta respuesta = deserializarRespuesta(respuesta_serializada);
        
        printf("Respuesta del servidor: %s\n", respuesta.respuesta);

        // Mostrar el tablero actual
        printf("Tablero actual:\n");

        for (const auto &fila : respuesta.tableroActual) {
            for (char letra : fila) {
                printf("%c ", letra);
            }
            printf("\n");
        }

        // Liberar la memoria de consulta y respuesta
        if (consulta.consulta) {
            delete[] consulta.consulta;
        }
        if (respuesta.respuesta) {
            delete[] respuesta.respuesta;
        }
    }
    close(sock);
    return 0;
}

void mostrarAyuda() {
    printf("Uso: ./Cliente -n nickname -p puertoServidor -s servidor\n");
    printf("     ./Cliente --nickname nickname --puerto puertoServidor --servidor servidor\n");
    printf("     ./Cliente -h\n");
    printf("     ./Cliente --help\n");
}

vector<char> recibirBuffer(int socket) {
    int size;
    // Recibir el tamaño del buffer
    int bytesReceived = recv(socket, &size, sizeof(int), 0);
    if (bytesReceived <= 0) {
        if (bytesReceived == 0) {
            // El cliente se desconectó
            cerr << "El Servidor se ha desconectado." << endl;
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
            cerr << "El Servidor se ha desconectado." << endl;
        } else {
            // Error en la recepción
            perror("Error en recv");
        }
        return {};
    }
    return buffer;
}

bool enviarBuffer(int socket, const vector<char> &buffer) {
    int size = buffer.size();
    // Enviar el tamaño del buffer
    int bytesSent = send(socket, &size, sizeof(int), 0);
    if (bytesSent <= 0) {
        if (bytesSent == 0) {
            // El servidor se desconectó
            cerr << "El Servidor se ha desconectado." << endl;
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
            // El Servidor se desconectó
            cerr << "El Servidor se ha desconectado." << endl;
        } else {
            // Error en el envío
            perror("Error en send");
        }
        return false;
    }
    return true;
}

vector<char> serializarAdivinanza(const Adivinanza &adv) {
    vector<char> buffer(sizeof(Adivinanza));
    memcpy(buffer.data(), &adv, sizeof(Adivinanza));
    return buffer;
}

Adivinanza deserializarAdivinanza(const vector<char> &buffer) {
    Adivinanza adv;
    memcpy(&adv, buffer.data(), sizeof(Adivinanza));
    return adv;
}

vector<char> serializarConsulta(const Consulta &con) {
    vector<char> buffer;
    for (const auto &row : con.tableroActual) {
        buffer.insert(buffer.end(), row.begin(), row.end());
    }
    if (con.consulta) {
        int consulta_len = strlen(con.consulta) + 1;
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&consulta_len), reinterpret_cast<const char*>(&consulta_len) + sizeof(int));
        buffer.insert(buffer.end(), con.consulta, con.consulta + consulta_len);
    } else {
        int consulta_len = 0;
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&consulta_len), reinterpret_cast<const char*>(&consulta_len) + sizeof(int));
    }
    return buffer;
}

Consulta deserializarConsulta(const vector<char> &buffer) {
    Consulta con;
    con.tableroActual = vector<vector<char>>(4, vector<char>(4));
    int index = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            con.tableroActual[i][j] = buffer[index++];
        }
    }
    int consulta_len;
    memcpy(&consulta_len, buffer.data() + index, sizeof(int));
    index += sizeof(int);
    if (consulta_len > 0) {
        con.consulta = new char[consulta_len];
        memcpy(con.consulta, buffer.data() + index, consulta_len);
    } else {
        con.consulta = nullptr;
    }
    return con;
}

Respuesta deserializarRespuesta(const vector<char> &buffer) {
    Respuesta resp;
    resp.tableroActual = vector<vector<char>>(4, vector<char>(4));
    int index = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            resp.tableroActual[i][j] = buffer[index++];
        }
    }
    int respuesta_len;
    memcpy(&respuesta_len, buffer.data() + index, sizeof(int));
    index += sizeof(int);
    if (respuesta_len > 0) {
        resp.respuesta = new char[respuesta_len];
        memcpy(resp.respuesta, buffer.data() + index, respuesta_len);
    } else {
        resp.respuesta = nullptr;
    }
    return resp;
}

void mostrarTableroEnCliente(const vector<vector<char>>& tablero) {
    
    vector<vector<char>> tableroResultado(4, vector<char>(4, '*'));

    for (int i = 0; i < 4; ++i) {
        cout << i << " ";
        for (int j = 0; j < 4; ++j)
                cout << tablero[i][j] << " ";
        cout << "\n";
    }
}