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

#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>

using namespace std;

const int CREACION_EXITOSA = 0;
const int ERROR_CREACION_FORK = -1;
const int ERROR_PARAMETROS = -2;

const int SEGUNDOS_ESPERA_PROCESO_PADRE = 10;
const int SEGUNDOS_ESPERA_PROCESOS_HIJOS = 10;
const int SEGUNDOS_ESPERA_PROCESOS_NIETOS = 10;
const int SEGUNDOS_ESPERA_PROCESOS_BIZNIETOS = 10;
const int SEGUNDOS_ESPERA_PROCESOS_ZOMBIES = 5;

const char* NOMBRE_PROCESO_PADRE = "Padre";
const char* NOMBRE_PROCESO_PRIMER_HIJO = "Hijo 1";
const char* NOMBRE_PROCESO_SEGUNDO_HIJO = "Zombie";
const char* NOMBRE_PROCESO_TERCER_HIJO = "Hijo 3";
const char* NOMBRE_PROCESO_PRIMER_NIETO = "Nieto 1";
const char* NOMBRE_PROCESO_SEGUNDO_NIETO = "Nieto 2";
const char* NOMBRE_PROCESO_TERCER_NIETO = "Nieto 3";
const char* NOMBRE_PROCESO_CUARTO_NIETO = "Demonio";
const char* NOMBRE_PROCESO_BIZNIETO = "Biznieto";

void ayuda ();
void convertirEnDemonio();
int crearFork ();
void esperarEnterUsuario ();
void evitarInteraccionConLaTerminal();
void ignorarSenialesDeTerminal();
void informarProcesoActual (string nombreProceso, int idProceso, int idProcesoPadre);
string obtenerNombreProceso(pid_t pid);
int validarParametroAyuda (int cantidadDeParametros, string valorParametro);

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

void convertirEnDemonio() {

    ignorarSenialesDeTerminal();

    // El proceso padre sale del programa, dejando al proceso hijo en ejecución.
    if (fork() > 0) {
        exit(0);
    }

    /*setsid(): Crea una nueva sesión y establece al proceso actual como líder de esa sesión. 
    Esto asegura que el proceso ya no tenga una terminal de control, lo que es esencial para que 
    un demonio funcione correctamente.*/
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    /* Se establece una señal para ignorar la señal SIGHUP, que generalmente se envía al cierre de la terminal. 
    Ignorar esta señal evita que el proceso demonio se detenga cuando la terminal se cierra.*/
    signal(SIGHUP, SIG_IGN);

    /* Este segundo fork es para garantizar que el proceso demonio no pueda adquirir un terminal de control 
    en el futuro. Una vez que se realiza este fork, el proceso es garantizado para no ser el líder de ningún 
    grupo de sesiones.*/
    if (fork() > 0) {
        exit(0);
    }

    // Cambio el directorio de trabajo actual del demonio al directorio raíz ("/"). 
    chdir("/");

    /* Establezco la máscara de modo de archivo para el proceso demonio a cero, lo que significa que todos 
    los permisos están permitidos.*/
    umask(0);

    /* Cierro todos los descriptores de archivo heredados, asegurando que el demonio no tenga abiertos
    descriptores de archivo de sus procesos padres.*/
    for (int fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--) {
        close(fd);
    }

    evitarInteraccionConLaTerminal();
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

void evitarInteraccionConLaTerminal()
{
    // Reabro los descriptores de archivo estándar (stdin, stdout, stderr) y los redirigen a /dev/null
    open("/dev/null", O_RDWR);  // stdin
    dup(0);                     // stdout
    dup(0);                     // stderr
}

void ignorarSenialesDeTerminal()
{
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
}

void informarProcesoActual (string nombreProceso, int idProceso, int idProcesoPadre)
{
    cout << "Soy el proceso " << nombreProceso << " con PID " << idProceso << ", mi padre es " << idProcesoPadre << endl; 
}

string obtenerNombreProceso(pid_t pid) {
    string ruta = "/proc/" + to_string(pid) + "/comm";
    ifstream archivoProceso(ruta);
    if (archivoProceso.is_open()) {
        string nombre;
        getline(archivoProceso, nombre);
        return nombre;
    } else {
        return "Unknown";
    }
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
    
    cout << "Por favor, aguarde unos segundos hasta que se muestre la jerarquia completa ..." << endl;
    /*=============== Estoy en el proceso Padre ===============*/
    prctl(PR_SET_NAME, NOMBRE_PROCESO_PADRE, 0, 0, 0);
    informarProcesoActual(obtenerNombreProceso(getpid()), getpid(), getppid());
    sleep(SEGUNDOS_ESPERA_PROCESO_PADRE);
    int estado;
    
    pid_t pidHijo1 = fork();
    
    if(pidHijo1 == CREACION_EXITOSA)
    {
        /*=============== Estoy en el proceso Hijo 1 ===============*/
        prctl(PR_SET_NAME, NOMBRE_PROCESO_PRIMER_HIJO, 0, 0, 0);
        informarProcesoActual(obtenerNombreProceso(getpid()), getpid(), getppid());
        sleep(SEGUNDOS_ESPERA_PROCESOS_HIJOS);

        if (crearFork() == CREACION_EXITOSA)
        {
            /*=============== Estoy en el proceso Nieto 1 ===============*/
            prctl(PR_SET_NAME, NOMBRE_PROCESO_PRIMER_NIETO, 0, 0, 0);
            informarProcesoActual(obtenerNombreProceso(getpid()), getpid(), getppid());
            sleep(SEGUNDOS_ESPERA_PROCESOS_NIETOS);
        }
        else
        {
            /*=============== Estoy en el proceso Hijo 1 ===============*/
            if(crearFork() == CREACION_EXITOSA)
            {
                /*=============== Estoy en el proceso Nieto 2 ===============*/
                prctl(PR_SET_NAME, NOMBRE_PROCESO_SEGUNDO_NIETO, 0, 0, 0);
                informarProcesoActual(obtenerNombreProceso(getpid()), getpid(), getppid());
                sleep(SEGUNDOS_ESPERA_PROCESOS_NIETOS);
            }
            else
            {
                /*=============== Estoy en el proceso Hijo 1 ===============*/
                if(crearFork() == CREACION_EXITOSA)
                {                    
                    /*=============== Estoy en el proceso Nieto 3 ===============*/
                    prctl(PR_SET_NAME, NOMBRE_PROCESO_TERCER_NIETO, 0, 0, 0);
                    informarProcesoActual(obtenerNombreProceso(getpid()), getpid(), getppid());

                    if (crearFork() == CREACION_EXITOSA)
                    {
                        /*=============== Estoy en el proceso Biznieto ===============*/
                        prctl(PR_SET_NAME, NOMBRE_PROCESO_BIZNIETO, 0, 0, 0);
                        informarProcesoActual(obtenerNombreProceso(getpid()), getpid(), getppid());
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
            prctl(PR_SET_NAME, NOMBRE_PROCESO_SEGUNDO_HIJO, 0, 0, 0);
            informarProcesoActual(obtenerNombreProceso(getpid()), getpid(), getppid()); 
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
                prctl(PR_SET_NAME, NOMBRE_PROCESO_TERCER_HIJO, 0, 0, 0);
                informarProcesoActual(obtenerNombreProceso(getpid()), getpid(), getppid());
                sleep(SEGUNDOS_ESPERA_PROCESOS_HIJOS);
                if (crearFork() == CREACION_EXITOSA)
                {
                    /*=============== Estoy en el proceso Demonio (Hijo del proceso Hijo 3) ===============*/
                    prctl(PR_SET_NAME, NOMBRE_PROCESO_CUARTO_NIETO, 0, 0, 0);
                    informarProcesoActual(obtenerNombreProceso(getpid()), getpid(), getppid());
                    convertirEnDemonio();
                }                
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
