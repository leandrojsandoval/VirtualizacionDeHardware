/*
#########################################################
#               Virtualizacion de hardware              #
#                                                       #
#   APL2 - Ejercicio 1                                  #
#   Nombre del script: Ejercicio1.cpp                   #
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

const int ERROR_CREACION_FORK = -1;

void ayuda ()
{
    cout << "============================== Ejercicio1.cpp ==============================" << endl;
    cout << "Este script crea una jerarquía de procesos como se describe en el enunciado:" << std::endl;
    cout << "" << endl;
    cout << "Padre" << endl; 
    cout << "|_ Hijo 1" << endl;
    cout << "|   |_ Nieto 1" << endl;
    cout << "|   |_ Nieto 2" << endl;
    cout << "|   |_ Nieto 3" << endl;
    cout << "|       |_ Biznieto" << endl;
    cout << "|" << endl;
    cout << "|_ Zombie" << endl;
    cout << "|" << endl;
    cout << "|_ Hijo 3" << endl;
    cout << "    |_ Demonio" << endl;
    cout << "" << endl;
    cout << "Uso: ./Ejercicio01 [opciones]" << endl;
    cout << "Opciones:" << endl;
    cout << "  -h, --help     Muestra este mensaje de ayuda." << endl;
    cout << "============================================================================" << endl;
}

int crearFork ()
{
    pid_t procesoHijo = fork();
    if(procesoHijo == -1) {
        perror("Error en la creacion de fork()");
        exit(ERROR_CREACION_FORK);
    }
    return procesoHijo;
}

void esperarTeclaUsuario ()
{
    sleep(1);
    cout << "Presione enter para finalizar..." << endl;
    getchar();
}

void informarProcesoActual (string nombreProceso) {
    cout << "Soy el proceso " << nombreProceso << " con PID " << getpid() << ", mi padre es " << getppid() << endl; 
}

int main (int argc, char* argv[])
{
    if (argc > 1)
    {
        string arg = argv[1];
        if (arg == "-h" || arg == "--help")
        {
            ayuda();
            return EXIT_SUCCESS;
        }
    }
    
    informarProcesoActual("Padre");
    
    if(crearFork() == 0)
    {
        if (crearFork() == 0)
        {
            informarProcesoActual("Nieto 1");
        } 
        else
        {
            informarProcesoActual("Hijo 1");

            if(crearFork() == 0)
            {
                informarProcesoActual("Nieto 2");
            }
            else
            {
                if(crearFork() == 0)
                {                    
                    informarProcesoActual("Nieto 3"); 

                    if (crearFork() == 0)
                    {
                        informarProcesoActual("Biznieto");
                    }
                    else
                    {
                        wait(NULL);     // Espero a Biznieto
                    }
                }
                else
                {
                    wait(NULL);         // Espero a Nieto 3 
                }
            }
            wait(NULL);                 // Espero a Nieto 2
        }
        wait(NULL);                     // Espero a Nieto 1
    }
    else
    {
        if(crearFork() == 0)
        {
            informarProcesoActual("Zombie"); 
        }
        else
        {
            if (crearFork() == 0)
            {
                informarProcesoActual("Hijo 3"); 
                
                if (crearFork() == 0)
                {
                    informarProcesoActual("Demonio"); 
                    setsid();
                }
                // No hay else ya que al proceso hijo lo saque de la sesion, por lo tanto Hijo 3 no tiene que esperar a nadie
            }
            else
            {
                wait(NULL);             // Espero a Hijo 3
                esperarTeclaUsuario();
            }
        }
        // No realizo un wait al proceso Zombie (Hijo 2)
    }
    return EXIT_SUCCESS;
}
