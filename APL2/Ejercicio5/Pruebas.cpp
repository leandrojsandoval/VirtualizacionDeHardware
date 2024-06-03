#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>

using namespace std;

// Función para generar el tablero con pares de letras aleatorias de A a Z
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

// Función para mostrar el tablero
void mostrarTablero(const vector<vector<char>>& tablero, const vector<vector<bool>>& descubiertas) {
    cout << "  0 1 2 3\n";
    for (int i = 0; i < 4; ++i) {
        cout << i << " ";
        for (int j = 0; j < 4; ++j) {
            if (descubiertas[i][j]) {
                cout << tablero[i][j] << " ";
            } else {
                cout << "* ";
            }
        }
        cout << "\n";
    }
}

// Función para verificar si el tablero está completo
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

int main() {
    srand(time(0)); // Semilla para el generador de números aleatorios

    // Generar el tablero
    vector<vector<char>> tablero = generarTablero();
    vector<vector<bool>> descubiertas(4, vector<bool>(4, false));

    while (!tableroCompleto(descubiertas)) {
        mostrarTablero(tablero, descubiertas);

        int fila1, col1, fila2, col2;
        cout << "Ingrese las coordenadas de la primera casilla (fila y columna): ";
        cin >> fila1 >> col1;
        cout << "Ingrese las coordenadas de la segunda casilla (fila y columna): ";
        cin >> fila2 >> col2;

        if (tablero[fila1][col1] == tablero[fila2][col2]) {
            descubiertas[fila1][col1] = true;
            descubiertas[fila2][col2] = true;
            cout << "¡Pareja encontrada!\n";
        } else {
            cout << "Las casillas no coinciden.\n";
        }
    }

    cout << "¡Felicidades! Has completado el juego.\n";
    mostrarTablero(tablero, descubiertas);

    return 0;
}
