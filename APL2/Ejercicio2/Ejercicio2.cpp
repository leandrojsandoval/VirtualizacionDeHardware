
/*
#########################################################
#               Virtualizacion de Hardware              #
#                                                       #
#   APL2 - Ejercicio 2                                  #
#   Nombre del script: Ejercicio2.cpp                   #
#                                                       #
#   Integrantes:                                        #
#                                                       #
#       Ocampo, Nicole Fabiana              44451238    #
#       Sandoval Vasquez, Juan Leandro      41548235    #
#       Tigani, Martin Sebastian            32788835    #
#       Villegas, Lucas Ezequiel            37792844    #
#       Vivas, Pablo Ezequiel               38703964    #
#                                                       #
#   Instancia de entrega: Reentrega                     #
#                                                       #
#########################################################
*/

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <filesystem>
#include <cstring>
#include <algorithm>
using namespace std;
namespace fs = filesystem;

void mostrar_ayuda(const string& nombre_programa) {
    cout << "Usos: " << nombre_programa << " -t <nivel_paralelismo> -i <directorio> [-o <archivo_salida>]" << endl;
    cout << "Parametros:" << endl;
    cout << "  -t, --threads   <nro>    Cantidad de threads a ejecutar concurrentemente para procesar los archivos del directorio (Requerido). El número ingresado debe ser un entero positivo." << endl;
    cout << "  -i, --input     <dir>    Ruta del directorio a analizar. (Requerido)" << endl;
    cout << "  -o, --output    <file>   Ruta del archivo con los resultados del procesamiento. (Opcional)" << endl;
    cout << "  -h, --help               Muestra esta ayuda y termina." << endl;
}

// Función para contar números en un archivo
void contar_numeros_en_archivo(const vector<string>& archivos, unordered_map<char, int>& conteo_numeros, mutex& mtx, int thread_id) {
    for (const auto& archivo : archivos) {
        ifstream file(archivo);
        if (!file.is_open()) {
            cerr << "No se pudo abrir el archivo: " << archivo << endl;
            continue;
        }

        unordered_map<char, int> conteo_local;
        char c;
        while (file.get(c)) {
            if (isdigit(c)) {
                conteo_local[c]++;
            }
        }
        file.close();

        // Imprimir los resultados del hilo y archivo actual
        {
            lock_guard<mutex> lock(mtx);
            cout << "Thread " << thread_id << ": Archivo leído " << archivo << ". Apariciones: ";
            for (char c = '0'; c <= '9'; ++c) {
                cout << c << "=" << conteo_local[c] << ", ";
            }
            cout << endl;

            for (const auto& [num, count] : conteo_local) {
                conteo_numeros[num] += count;
            }
        }
    }
}

void procesar_archivos_en_directorio(const string& directorio, int nivel_paralelismo, bool generar_archivo, const string& archivo_salida) {
    vector<thread> threads;
    unordered_map<char, int> conteo_numeros;
    mutex mtx;

    vector<string> archivos;
    for (const auto& entrada : fs::directory_iterator(directorio)) {
        if (entrada.path().extension() == ".txt") {
            archivos.push_back(entrada.path().string());
        }
    }

    size_t archivos_por_thread = archivos.size() / nivel_paralelismo;
    size_t archivos_restantes = archivos.size() % nivel_paralelismo;

    size_t inicio = 0;
    for (int i = 0; i < nivel_paralelismo; ++i) {
        size_t fin = inicio + archivos_por_thread + (i < archivos_restantes ? 1 : 0);
        vector<string> archivos_subset(archivos.begin() + inicio, archivos.begin() + fin);
        threads.emplace_back(contar_numeros_en_archivo, archivos_subset, ref(conteo_numeros), ref(mtx), i + 1);
        inicio = fin;
    }

    for (auto& thread : threads) {
        thread.join();
    }

    cout << "Finalizado lectura: Apariciones total: ";
    for (char c = '0'; c <= '9'; ++c) {
        cout << c << "=" << conteo_numeros[c] << ", ";
    }
    cout << endl;

    if (generar_archivo) {
        ofstream outfile(archivo_salida);
        if (outfile.is_open()) {
            for (char c = '0'; c <= '9'; ++c) {
                outfile << c << "=" << conteo_numeros[c] << endl;
            }
            outfile.close();
        } else {
            cerr << "No se pudo abrir el archivo de salida." << endl;
        }
    }
}

bool es_numero(const string& str) {
    return !str.empty() && all_of(str.begin(), str.end(), ::isdigit);
}

bool validarParametros(int argc, char* argv[], int& nivel_paralelismo, string& directorio, bool& generar_archivo, string& archivo_salida) {
    if (argc < 2) {
        mostrar_ayuda(argv[0]);
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-t" || arg == "--threads") {
            if (i + 1 < argc && es_numero(argv[i + 1])) {
                nivel_paralelismo = stoi(argv[++i]);
                if (nivel_paralelismo <= 0) {
                    cerr << "El número de threads debe ser un entero positivo." << endl;
                    return false;
                }
            } else {
                cerr << "Debe proporcionar un número de threads positivo después de " << arg << "." << endl;
                return false;
            }
        } else if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) {
                directorio = argv[++i];
                if (!fs::is_directory(directorio)) {
                    cerr << "La ruta proporcionada no es un directorio válido." << endl;
                    return false;
                }
            } else {
                cerr << "Debe proporcionar una ruta de directorio después de " << arg << "." << endl;
                return false;
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                generar_archivo = true;
                archivo_salida = argv[++i];
            } else {
                cerr << "Debe proporcionar una ruta de archivo después de " << arg << "." << endl;
                return false;
            }
        } else if (arg == "-h" || arg == "--help") {
            mostrar_ayuda(argv[0]);
            return false;
        }
    }

    if (nivel_paralelismo <= 0 || directorio.empty()) {
        cerr << "Parámetros inválidos. Asegúrese de proporcionar un nivel de paralelismo positivo y un directorio válido." << endl;
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    int nivel_paralelismo = 0;
    string directorio;
    bool generar_archivo = false;
    string archivo_salida;

    if (!validarParametros(argc, argv, nivel_paralelismo, directorio, generar_archivo, archivo_salida)) {
        return 1;
    }

    procesar_archivos_en_directorio(directorio, nivel_paralelismo, generar_archivo, archivo_salida);

    return 0;
}
