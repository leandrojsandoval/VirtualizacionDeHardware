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

const int CREACION_EXITOSA = 0;
const int ERROR_CREACION_FORK = -1;
const int ERROR_PARAMETROS = -2;

const int SEGUNDOS_ESPERA_PROCESO_PADRE = 10;
const int SEGUNDOS_ESPERA_PROCESOS_HIJOS = 10;
const int SEGUNDOS_ESPERA_PROCESOS_NIETOS = 10;
const int SEGUNDOS_ESPERA_PROCESOS_BIZNIETOS = 10;

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

void esperarEnterUsuario ()
{
    cout << "Presione enter para finalizar..." << endl;
    getchar();
}

void informarProcesoActual (string nombreProceso, int idProceso, int idProcesoPadre)
{
    cout << "Soy el proceso " << nombreProceso << " con PID " << idProceso << ", mi padre es " << idProcesoPadre << endl; 
}

int validarParametroAyuda (int cantidadDeParametros, string valorParametro)
{
    if (cantidadDeParametros == 2)
    {
        if (valorParametro == "-h" || valorParametro == "--help")
        {
            ayuda();
            return EXIT_SUCCESS;
        }
        else
        {
            cout << "El parametro indicado no corresponde al parametro ayuda (-h o --help)" << endl;
            return ERROR_PARAMETROS;
        }
    } 
    else
    {
        cout << "El programa solo puede recibir el parametro de ayuda (-h o --help)" << endl;
        return ERROR_PARAMETROS;
    }
}

int main (int argc, char* argv[])
{
    if(argc > 1) 
    {
        return validarParametroAyuda(argc, argv[1]);
    }
    
    /*=============== Estoy en el proceso Padre ===============*/
    informarProcesoActual("Padre", getpid(), getppid());
    sleep(SEGUNDOS_ESPERA_PROCESO_PADRE);
    int estado;
    
    pid_t pidHijo1 = fork();
    
    if(pidHijo1 == CREACION_EXITOSA)
    {
        /*=============== Estoy en el proceso Hijo 1 ===============*/
        informarProcesoActual("Hijo 1", getpid(), getppid());
        sleep(SEGUNDOS_ESPERA_PROCESOS_HIJOS);

        if (crearFork() == CREACION_EXITOSA)
        {
            /*=============== Estoy en el proceso Nieto 1 ===============*/
            informarProcesoActual("Nieto 1", getpid(), getppid());
            sleep(SEGUNDOS_ESPERA_PROCESOS_NIETOS);
        }
        else
        {
            /*=============== Estoy en el proceso Hijo 1 ===============*/
            if(crearFork() == CREACION_EXITOSA)
            {
                /*=============== Estoy en el proceso Nieto 2 ===============*/
                informarProcesoActual("Nieto 2", getpid(), getppid());
                sleep(SEGUNDOS_ESPERA_PROCESOS_NIETOS);
            }
            else
            {
                /*=============== Estoy en el proceso Hijo 1 ===============*/
                if(crearFork() == CREACION_EXITOSA)
                {                    
                    /*=============== Estoy en el proceso Nieto 3 ===============*/
                    informarProcesoActual("Nieto 3", getpid(), getppid());

                    if (crearFork() == CREACION_EXITOSA)
                    {
                        /*=============== Estoy en el proceso Biznieto ===============*/
                        informarProcesoActual("Biznieto", getpid(), getppid());
                        sleep(SEGUNDOS_ESPERA_PROCESOS_BIZNIETOS);
                    }
                    else
                    {
                        /*=============== Estoy en el proceso Nieto 3 ===============*/

                        // Espero a Biznieto
                        wait(NULL);
                    }
                }
                else
                {
                    /*=============== Estoy en el proceso Hijo 1 ===============*/

                    // Espero a Nieto 3
                    wait(NULL);
                    // Espero a Nieto 2
                    wait(NULL);
                    // Espero a Nieto 1
                    wait(NULL);
                }
            }
        }
    }
    else if(pidHijo1 > 0) 
    {
        /*=============== Estoy en el proceso Padre ===============*/
        if(crearFork() == CREACION_EXITOSA)
        {
            /*=============== Estoy en el proceso Zombie (Hijo 2) ===============*/
            informarProcesoActual("Zombie", getpid(), getppid()); 
            sleep(SEGUNDOS_ESPERA_PROCESOS_ZOMBIES);
            exit(0);
        }
        else
        {            
            /*=============== Estoy en el proceso Padre ===============*/
            pid_t pidHijo3 = fork();
            if(pidHijo3 == CREACION_EXITOSA)
            {
                /*=============== Estoy en el proceso Hijo 3 ===============*/
                informarProcesoActual("Hijo 3", getpid(), getppid());
                sleep(SEGUNDOS_ESPERA_PROCESOS_HIJOS);
                
                //////////////////////////////////////////////////
                if (crearFork() == CREACION_EXITOSA)
                {
                    /*=============== Estoy en el proceso Demonio (Hijo del proceso Hijo 3) ===============*/
                    informarProcesoActual("Demonio", getpid(), getppid());
                    sleep(SEGUNDOS_ESPERA_PROCESOS_DEMONIOS);
                    setsid();
                    chdir("/");
                }
                //////////////////////////////////////////////////
                
                // No hay else ya que al proceso hijo lo saque de la sesion, por lo tanto Hijo 3 no tiene que esperar a nadie
            }
            else if(pidHijo3 > 0)
            {
                /*=============== Estoy en el proceso Padre ===============*/
                
                // Debo especificar que pid tengo que esperar porque podria hacer un wait del proceso Zombie (Hijo 2)

                // Espero a Hijo 3
                waitpid(pidHijo3, &estado, 0);
                // Espero a Hijo 1
                waitpid(pidHijo1, &estado, 0);

                sleep(SEGUNDOS_ESPERA_PROCESO_PADRE);
                esperarEnterUsuario();
            }
            else
            {
                cout << "Hubo un error en la creacion del fork" << endl;
                return ERROR_CREACION_FORK;
            }
        }
    } 
    else 
    {
        cout << "Hubo un error en la creacion del fork" << endl;
        return ERROR_CREACION_FORK;
    }
    return EXIT_SUCCESS;
}
