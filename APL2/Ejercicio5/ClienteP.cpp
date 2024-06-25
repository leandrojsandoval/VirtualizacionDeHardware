#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <iostream>

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

vector<char> recibirBuffer(int socket);
void enviarBuffer(int socket, const vector<char> &buffer);
Adivinanza deserializarAdivinanza(const vector<char> &buffer);
Consulta deserializarConsulta(const vector<char> &buffer);
vector<char> serializarAdivinanza(const Adivinanza &adv);
vector<char> serializarConsulta(const Consulta &con);
Respuesta deserializarRespuesta(const vector<char> &buffer);

void mostrarTableroEnCliente(const vector<vector<char>>& tablero);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <IP_del_servidor> <puerto>\n", argv[0]);
        return -1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    vector<char> buffer(BUFFER_SIZE);

    // Crear el socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error al crear el socket \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convertir direcciones IPv4 e IPv6 de texto a binario
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        printf("\n Dirección no soportada \n");
        return -1;
    }

    // Conectarse al servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Error de conexión \n");
        return -1;
    }

    printf("Conectado al servidor\n");

    

    // Esperar el mensaje de inicio del juego
    valread = recv(sock, buffer.data(), BUFFER_SIZE, 0);
    printf("%s\n", buffer.data());

    // Comunicación con el servidor
    bool juegoTerminado=false;
    while (!juegoTerminado) {
        // Esperar hasta que el servidor indique que es el turno del cliente
        vector<char> consulta_serializada = recibirBuffer(sock);
        Consulta consulta = deserializarConsulta(consulta_serializada);
        
        if (strcmp(consulta.consulta, "El juego ha terminado\n") == 0) {
            printf("El juego ha terminado\n");
            juegoTerminado = true;
            break;
        }

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
        enviarBuffer(sock, adivinanza_serializada);

        // Esperar la respuesta del servidor
        vector<char> respuesta_serializada = recibirBuffer(sock);
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
    if(juegoTerminado){
        // Buffer para recibir el mensaje
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        // Recibir el mensaje del servidor
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received < 0) {
            perror("recv");
        } else {
            // Asegurarse de que la cadena está terminada en nulo
            buffer[bytes_received] = '\0';
            cout << "Resultado final: " << buffer << endl;
        }
    }

    close(sock);
    return 0;
}

vector<char> recibirBuffer(int socket) {
    int size;
    recv(socket, &size, sizeof(int), 0);
    vector<char> buffer(size);
    recv(socket, buffer.data(), size, 0);
    return buffer;
}

void enviarBuffer(int socket, const vector<char> &buffer) {
    int size = buffer.size();
    send(socket, &size, sizeof(int), 0);
    send(socket, buffer.data(), size, 0);
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