
/*
#########################################################
#               Virtualizacion de hardware              #
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
#   Instancia de entrega: Primera Entrega               #
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

using namespace std;

// Función para contar números en un archivo
void contar_numeros_en_archivo(const string& archivo, unordered_map<char, int>& conteo_numeros, mutex& mtx, int thread_id) {
    ifstream file(archivo);
    if (!file.is_open()) {
        cerr << "No se pudo abrir el archivo: " << archivo << endl;
        return;
    }

    unordered_map<char, int> conteo_local;
    char c;
    while (file.get(c)) {
        if (isdigit(c)) {
            conteo_local[c]++;
        }
    }

    file.close();

    // Imprimir resultados por hilo
    {
        lock_guard<mutex> lock(mtx);
        cout << "Thread " << thread_id << ": Archivo leído " << archivo << ". Apariciones: ";
        for (char c = '0'; c <= '9'; ++c) {
            cout << c << "=" << conteo_local[c] << ", ";
            conteo_numeros[c] += conteo_local[c];
        }
        cout << endl;
    }
}

// Función principal para procesar archivos en el directorio
void procesar_archivos_en_directorio(const string& directorio, int nivel_paralelismo, bool generar_archivo,  const string& archivo_salida) {
    vector<thread> threads;
    unordered_map<char, int> conteo_numeros;
    mutex mtx;

    vector<string> archivos;
    for (const auto& entrada : filesystem::directory_iterator(directorio)) {
        if (entrada.path().extension() == ".txt") {
            archivos.push_back(entrada.path().string());
        }
    }

    size_t archivo_count = archivos.size();
    for (size_t i = 0; i < archivo_count; i += nivel_paralelismo) {
        size_t end = min(i + nivel_paralelismo, archivo_count);
        for (size_t j = i; j < end; ++j) {
            threads.emplace_back(contar_numeros_en_archivo, archivos[j], ref(conteo_numeros), ref(mtx), j+1);
        }

        // Esperar a que todos los hilos terminen
        for (auto& thread : threads) {
            thread.join();
        }
        threads.clear();
    }

    // Imprimir resultados totales
    cout << "Finalizado lectura: Apariciones total: ";
    for (char c = '0'; c <= '9'; ++c) {
        cout << c << "=" << conteo_numeros[c] << ", ";
    }
    cout << endl;

    // Generar archivo de salida si se especifica
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
int main(int argc, char* argv[]) {
    if (argc < 5) {
        cerr << "Uso: " << argv[0] << " -t <nivel_paralelismo> -i <directorio> [-o <archivo_salida>]" << endl;
        return 1;
    }

    int nivel_paralelismo = 0;
    string directorio;
    bool generar_archivo = false;
    string archivo_salida;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-t" || arg == "--threads") {
            nivel_paralelismo = stoi(argv[++i]);
        } else if (arg == "-i" || arg == "--input") {
            directorio = argv[++i];
        } else if (arg == "-o" || arg == "--output") {
            generar_archivo = true;
            archivo_salida = argv[++i];
        }
    }

    if (nivel_paralelismo <= 0 || directorio.empty()) {
        cerr << "Parámetros inválidos. Asegúrese de proporcionar un nivel de paralelismo positivo y un directorio válido." << endl;
        return 1;
    }

    procesar_archivos_en_directorio(directorio, nivel_paralelismo, generar_archivo, archivo_salida);

    return 0;
}
