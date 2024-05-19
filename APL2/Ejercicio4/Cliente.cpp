#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <cerrno>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <cstring>
#include <signal.h>
#include <vector>

typedef struct {
    char situacion[5]; // ALTA (ingreso/rescatado) BAJA (adopcion/egreso)
    char nombre[21];
    char raza[21];
    char sexo[2];    // M o H
    char estado[3]; // CA (castrado) o SC (sin castrar) 
}gato;

typedef struct {
	int baja;
	int alta;
	int consultar;
	char notibaja[100];
	char notialta[100];
	char consulta[100];
    gato g;
    char rescatados[20]; //nombre del archivo que se creara cada ves que se inicie una consulta general
}acciones;

sem_t* semaforos[5];
/*
    0 - Servidor (solo puede haber 1) se inicia en 1 y rapidamente se ocupa por el primer demonio. se liberara cuando el servidor demonio sea detenido
    1 - Cliente, el cual se liberara justo antes de comenzar el bucle infinito, se inicia en 0.
    2 - MC, (solo puede acceder un proceso por vez) se inicia en 1
    3 - TC, iniciara en 0, y se liberara antes de comenzar el bucle y al finalizar un ciclo de dicho bucle. 
    4 - TS, iniciara en 0 y solo se liberara cuando el cliente termine su actividad
*/

using namespace std;

#define NombreMemoria "miMmemoria"

void inicializarSemaforos();
void cerrar_Sem();
void liberar_semaforos();
bool Ayuda(const char *);
acciones* abrir_mem_comp();
void cerrar_mem_comp(acciones*);
int leer_rescatados(const char[]);
bool validar_parametro(const char[]);
int main(int argc, char *argv[]){
    if(argc == 1){
        cout << "Error, la script debe recibir parametros, para mas información consulte la ayuda" << endl;
        cout << "./Cliente -h o -/Cliente --help" << endl;
        exit(EXIT_FAILURE);
    }

    if((strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0) && argc == 2){
        Ayuda(argv[1]);
        exit(EXIT_SUCCESS);
    }
    if(strcmp(argv[1],"ALTA") != 0 && strcmp(argv[1],"BAJA") != 0 && strcmp(argv[1],"CONSULTA") != 0){
        cout << "Error, parametro inválido" << endl;
        exit(EXIT_FAILURE);
    }
    inicializarSemaforos();
    //  P(Cliente)
    sem_wait(semaforos[1]);
    //  P(TC)
    sem_wait(semaforos[3]);
    if(strcmp(argv[1],"ALTA") == 0){
        if(argc == 6){
            if(validar_parametro(argv[2]) == false || validar_parametro(argv[3]) == false){
                sem_post(semaforos[3]);
                //V(TC)
                sem_post(semaforos[1]);
                //V(Cliente)
                cerrar_Sem();
                exit(EXIT_FAILURE);
            }
            if(strcmp(argv[4],"M") != 0 && strcmp(argv[4],"H") != 0){
                cout << "Error, el sexo debe ser Macho (M) o Hembra (H)" << endl;
                sem_post(semaforos[3]);
                //V(TC)
                sem_post(semaforos[1]);
                //V(Cliente)
                cerrar_Sem();
                exit(EXIT_FAILURE);
             }
            if(strcmp(argv[5],"CA") != 0 && strcmp(argv[5],"SC") != 0){
                cout << "Error, el estado debe ser Castrado (CA) o Sin castrar (SC)" << endl;
                sem_post(semaforos[3]);
                //V(TC)
                sem_post(semaforos[1]);
                //V(Cliente)
                cerrar_Sem();
                exit(EXIT_FAILURE);
            }
            // P(MC)
            sem_wait(semaforos[2]);
            acciones *a = abrir_mem_comp();
            strcpy(a->g.situacion,argv[1]);
            strcpy(a->g.nombre,argv[2]);
            strcpy(a->g.raza,argv[3]);
            strcpy(a->g.sexo,argv[4]);
            strcpy(a->g.estado,argv[5]);
            a->alta = 1;
            cerrar_mem_comp(a);

            sem_post(semaforos[2]);
            // V(MC)    liberamos la memoria compartida
            sem_post(semaforos[4]);
            // V(TS)    le damos el turno al servidor
            sem_wait(semaforos[3]);
            //P(TC)     bloqueamos el turno del cliente, como este estara en 0 en este momento, se bloqueara el proceso a la espera de que el servidor termine su turno.
            sem_wait(semaforos[2]);
            // P(MC)
            a = abrir_mem_comp();
            if(strcmp(a->notialta,"") != 0){
                string tmp_notialta(a->notialta);   
                cout << tmp_notialta << endl;   // el servidor le notifico al cliente que el nombre del gato ya existe.
                strcpy(a->notialta,"");
            }
            cerrar_mem_comp(a);
            sem_post(semaforos[2]);
            // V(MC)
        }
        else{
            cout << "Error, cantidad de parametros erronea junto a la acción alta." << endl;
            sem_post(semaforos[3]);
            //V(TC)
            sem_post(semaforos[1]);
            //V(Cliente)
            cerrar_Sem();
            exit(EXIT_FAILURE);
        }
    }
    if(strcmp(argv[1],"BAJA") == 0){
        if(argc == 3){
            if(validar_parametro(argv[2]) == false){
                sem_post(semaforos[3]);
                //V(TC)
                sem_post(semaforos[1]);
                //V(Cliente)
                cerrar_Sem();
                exit(EXIT_FAILURE);
            }
            sem_wait(semaforos[2]);
            // P(MC)
            acciones *a = abrir_mem_comp();
            strcpy(a->g.nombre,argv[2]);
            a->baja = 1;
            cerrar_mem_comp(a);
            sem_post(semaforos[2]);
            // V(MC)
            sem_post(semaforos[4]);
            // V(TS)
            sem_wait(semaforos[3]);
            // P(TC)
            sem_wait(semaforos[2]);
            // P(MC)
            a = abrir_mem_comp();
            if(strcmp(a->notibaja,"") != 0){
                string tmp_notibaja(a->notibaja);
                cout << tmp_notibaja << endl;
                strcpy(a->notibaja,"");
            }
            cerrar_mem_comp(a);
            sem_post(semaforos[2]);
            // V(MC)
        }
        else{
            cout << "Error, cantidad de parametros erronea junto a la acción baja" << endl;
            sem_post(semaforos[3]);
            //V(TC)
            sem_post(semaforos[1]);
            //V(Cliente)
            cerrar_Sem();
            exit(EXIT_FAILURE);
        }
    }

    if(strcmp(argv[1],"CONSULTA") == 0){
        if(argc == 3){
            //En caso de mandar un nombre en concreto...
            if(validar_parametro(argv[2]) == false){
                sem_post(semaforos[3]);
                //V(TC)
                sem_post(semaforos[1]);
                //V(Cliente)
                cerrar_Sem();
                exit(EXIT_FAILURE);
            }
            sem_wait(semaforos[2]);
            // P(MC)
            acciones *a = abrir_mem_comp();
            strcpy(a->g.nombre,argv[2]);
            a->consultar = 1;
            cerrar_mem_comp(a);
            sem_post(semaforos[2]);
            // V(MC)

            sem_post(semaforos[4]);
            // V(TS)
            sem_wait(semaforos[3]);
            // P(TC)
            sem_wait(semaforos[2]);
            // P(MC)
            a = abrir_mem_comp();
            if(strcmp(a->consulta,"") != 0){
                string tmp_consulta(a->consulta);
                cout << tmp_consulta << endl;
                strcpy(a->consulta,"");
            }
            else{
                    string tmp_situacion(a->g.situacion);
                    string tmp_nombre(a->g.nombre);
                    string tmp_raza(a->g.raza);
                    string tmp_sexo(a->g.sexo);
                    string tmp_estado(a->g.estado);
                    cout << "Situacion: " + tmp_situacion << endl;
                    cout << "Nombre: " + tmp_nombre << endl;
                    cout << "Raza: " + tmp_raza << endl;
                    cout << "Sexo: " + tmp_sexo << endl;
                    cout << "Estado: " + tmp_estado << endl;
                    strcpy(a->g.situacion,"");
                    strcpy(a->g.nombre,"");
                    strcpy(a->g.raza,"");
                    strcpy(a->g.sexo,"");
                    strcpy(a->g.estado,"");
            }
            cerrar_mem_comp(a);
            sem_post(semaforos[2]);
            // V(MC)
        }
        else{
            if(argc == 2){
                sem_wait(semaforos[2]);
                // P(MC)
                acciones *a = abrir_mem_comp();
                strcpy(a->rescatados,"rescatados.txt");
                a->consultar = 1;
                cerrar_mem_comp(a);
                sem_post(semaforos[2]);
                //V(MC)
                sem_post(semaforos[4]);
                // V(TS)
                sem_wait(semaforos[3]);
                // P(TC)
                sem_wait(semaforos[2]);
                // P(MC)
                a = abrir_mem_comp();
                if(strcmp(a->consulta,"") != 0){
                    string tmp_consulta(a->consulta);
                    cout << tmp_consulta << endl;
                    strcpy(a->consulta,"");
                }
                else
                    int res = leer_rescatados(a->rescatados);
                cerrar_mem_comp(a);
                sem_post(semaforos[2]);
                // V(MC)
            }
            else{
                cout << "Error, cantidad de parametros erronea junto a la acción consultar" << endl;
                sem_post(semaforos[3]);
                //V(TC)
                sem_post(semaforos[1]);
                //V(Cliente)
                cerrar_Sem();
                exit(EXIT_FAILURE);
            }
        }
    }
    // V(Cliente)
    sem_post(semaforos[1]);
    // V(TC)
    sem_post(semaforos[3]);
    cerrar_Sem();
    return EXIT_SUCCESS;
}

bool Ayuda(const char *cad)
{
    if (!strcmp(cad, "-h") || !strcmp(cad, "--help") )
    {
        cout << "Esta script permite solicitar al proceso servidor diferentes acciones."<< endl;
        cout << "Alta, si se quiere registrar un gato"<< endl;
        cout << "La manera de ejecutar este modo es ./Cliente ALTA [nombre del gato] [raza] [sexo (M/H)] [castrado(CA)/sin castrar(SC)]"<< endl;
        cout << "Baja, si un gato fue adoptado hay que darle la baja" << endl;
        cout << "La manera de ejecutar este modo es ./Cliente BAJA [nombre del gato]" << endl;
        cout << "Consulta, si se quiere saber sobre un gato en específico o todos aquellos gatos que han sido rescatados" << endl;
        cout << "La manera de saber sobre un gato en específico es: ./Cliente CONSULTA [nombre del gato]" << endl;
        cout << "La manera de saber sobre todos los gatos que han sido rescatados es: ./Cliente CONSULTA" << endl;
        cout << "Para finalizar el proceso servidor simplemente basta con ejecutar ./Disparador" << endl;
        return true;
    }
    return false;
}

void inicializarSemaforos(){
    semaforos[0] = sem_open("servidor",O_CREAT,0600,1);
    int valorSemServi = 65;
    sem_getvalue(semaforos[0],&valorSemServi);
    if(valorSemServi == 1){
        cout << "Error, servidor inactivo." << endl;
        cout << "Intente mas tarde" << endl;
        sem_close(semaforos[0]);
        sem_unlink("servidor");
        exit(EXIT_FAILURE);
    }
    semaforos[1] = sem_open("cliente",O_CREAT,0600,0);
    semaforos[2] = sem_open("memComp",O_CREAT,0600,1);
    semaforos[3] = sem_open("t_Cliente",O_CREAT,0600,0);
    semaforos[4] = sem_open("t_Servidor",O_CREAT,0600,0);
}

void cerrar_Sem(){
    sem_close(semaforos[0]);
    sem_close(semaforos[1]);
    sem_close(semaforos[2]);
    sem_close(semaforos[3]);
    sem_close(semaforos[4]);
}

void liberar_semaforos(){
    sem_unlink("cliente");
    sem_unlink("memComp");
    sem_unlink("t_Cliente");
    sem_unlink("t_Servidor");
}

acciones* abrir_mem_comp(){
    int idMemoria = shm_open(NombreMemoria, O_CREAT | O_RDWR, 0600); // obtenemos un numero que nos identifica esta memoria.

    // definir nuestra variable que es la variable que estara mapeada a memoria compartida.
    // la memoria compartida ya esta creada y tenermos un identificador, pero no podemos accederla..

    //me va a mapear, o a relacionar un segmento de memoria a una variable. agarra un espacio de memoria y mapearlo/darnos la direccion de memoria de ese espacio de memoria.
    acciones *memoria = (acciones *)mmap(NULL, sizeof(acciones), PROT_READ | PROT_WRITE, MAP_SHARED, idMemoria,0);

    close(idMemoria);
    return memoria;
}

void cerrar_mem_comp(acciones *a){
    munmap(a, sizeof(acciones));
}

int leer_rescatados(const char path[20]){
    ifstream archivo;
    string texto;
    string tmp_path(path);
    archivo.open(tmp_path,ios::in);
    if(archivo.fail()){
        cout << "no se pudo abrir el archivo" << endl;
        return -1;
    }
    cout << "Gatos rescatados" << endl;
    while(getline(archivo,texto))
        cout << texto << endl;
    archivo.close();
    remove(path);
    return 0;
}

bool validar_parametro(const char nombre[]){
    string tmp_nombre(nombre);
    if(tmp_nombre.length() > 20){
        cout << "Error, el nombre del gato no debe sobrepasar los 20 caracteres" << endl;
        return false;
    }
    return true;
}