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
const int CREACION_EXITOSA = 0;

const int SEGUNDOS_ESPERA_PROCESO_PADRE = 10;
const int SEGUNDOS_ESPERA_PROCESOS_HIJOS = 15;
const int SEGUNDOS_ESPERA_PROCESOS_NIETOS = 10;
const int SEGUNDOS_ESPERA_PROCESOS_BIZNIETOS = 20;

const int SEGUNDOS_ESPERA_PROCESOS_ZOMBIES = 5;
const int SEGUNDOS_ESPERA_PROCESOS_DEMONIOS = 5;

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

void informarProcesoActual (string nombreProceso, int idProceso, int idProcesoPadre) {
    cout << "Soy el proceso " << nombreProceso << " con PID " << idProceso << ", mi padre es " << idProcesoPadre << endl; 
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
    
    informarProcesoActual("Padre", getpid(), getppid());
    sleep(SEGUNDOS_ESPERA_PROCESO_PADRE);
    
    if(crearFork() == CREACION_EXITOSA)
    {
        informarProcesoActual("Hijo 1", getpid(), getppid());
        sleep(SEGUNDOS_ESPERA_PROCESOS_HIJOS);

        if (crearFork() == CREACION_EXITOSA)
        {
            informarProcesoActual("Nieto 1", getpid(), getppid());
            sleep(SEGUNDOS_ESPERA_PROCESOS_NIETOS);
        } 
        else
        {
            if(crearFork() == CREACION_EXITOSA)
            {
                informarProcesoActual("Nieto 2", getpid(), getppid());
                sleep(SEGUNDOS_ESPERA_PROCESOS_NIETOS);
            }
            else
            {
                if(crearFork() == CREACION_EXITOSA)
                {                    
                    informarProcesoActual("Nieto 3", getpid(), getppid());

                    if (crearFork() == CREACION_EXITOSA)
                    {
                        informarProcesoActual("Biznieto", getpid(), getppid());
                        sleep(SEGUNDOS_ESPERA_PROCESOS_BIZNIETOS);
                    }
                    else
                    {
                        // Espero a Biznieto
                        wait(NULL);
                    }
                }
                else
                {
                    // Espero a Nieto 3
                    wait(NULL);
                }
                // Espero a Nieto 2
                wait(NULL);
            }
            // Espero a Nieto 1
            wait(NULL);
        }
    }
    else
    {
        if(crearFork() == CREACION_EXITOSA)
        {
            informarProcesoActual("Zombie", getpid(), getppid()); 
            sleep(SEGUNDOS_ESPERA_PROCESOS_ZOMBIES);
            exit(0);
        }
        else
        {
            if (crearFork() == CREACION_EXITOSA)
            {
                informarProcesoActual("Hijo 3", getpid(), getppid());
                sleep(SEGUNDOS_ESPERA_PROCESOS_HIJOS);
                
                if (crearFork() == CREACION_EXITOSA)
                {
                    informarProcesoActual("Demonio", getpid(), getppid());
                    sleep(SEGUNDOS_ESPERA_PROCESOS_DEMONIOS);
                    setsid();
                    chdir("/");
                }
                // No hay else ya que al proceso hijo lo saque de la sesion, por lo tanto Hijo 3 no tiene que esperar a nadie
            }
            else
            {
                // Espero a Hijo 3
                wait(NULL);
                // Espero a Hijo 1
                wait(NULL);
                wait(NULL);
                // No realizo un wait al proceso Zombie (Hijo 2)
                sleep(SEGUNDOS_ESPERA_PROCESO_PADRE);
                esperarTeclaUsuario();
            }
        }
    }
    return EXIT_SUCCESS;
}
