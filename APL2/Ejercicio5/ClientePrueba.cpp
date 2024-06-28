#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

int main() {
    char server_ip[BUFFER_SIZE];
    int port;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Leer la IP y el puerto del servidor desde el teclado
    printf("Ingrese la IP del servidor: ");
    fgets(server_ip, BUFFER_SIZE, stdin);
    server_ip[strcspn(server_ip, "\n")] = 0; // Remover el salto de línea

    printf("Ingrese el puerto del servidor: ");
    scanf("%d", &port);
    getchar(); // Consumir el salto de línea restante

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
    valread = read(sock, buffer, BUFFER_SIZE);
    printf("%s\n", buffer);

    // Comunicación con el servidor
    while (1) {
        // Esperar hasta que el servidor indique que es el turno del cliente
        valread = recv(sock, buffer, BUFFER_SIZE, 0);
        if (valread <= 0) {
            printf("Servidor desconectado\n");
            break;
        }

        buffer[valread] = '\0';
        if (strcmp(buffer, "Es tu turno\n") == 0) {
            printf("Es tu turno. Escriba un mensaje: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = 0; // Remover el salto de línea

            // Enviar mensaje al servidor (bloqueante)
            send(sock, buffer, strlen(buffer), 0);

            // Esperar la respuesta del servidor (bloqueante)
            valread = recv(sock, buffer, BUFFER_SIZE, 0);
            if (valread <= 0) {
                printf("Servidor desconectado\n");
                break;
            }

            buffer[valread] = '\0';
            printf("Respuesta del servidor: %s\n", buffer);
        }
    }

    close(sock);
    return 0;
}
