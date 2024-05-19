#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <cerrno>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <cstring>
#include <signal.h>
#include <vector>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <linux/fs.h>
#include <sys/param.h>
#include <time.h>
#include <syslog.h>

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
//necesitamos un identificador para la memoria compartida para que los diferentes procesos que vayan a utilizarla tengan una manera de referenciarla
#define NombreMemoria "miMmemoria"
#define MemPid "pidServidor"
acciones* accion;


void realizar_Actividades();
/***********************************Semaforos**********************************/
void eliminar_Sem();
void inicializarSemaforos();
/***********************************Semaforos**********************************/

/***********************************Recursos**********************************/
void liberar_Recursos(int);
/***********************************Recursos**********************************/

/***********************************Archivos**********************************/
bool Ayuda(const char *);
int consultarArchivo(const char[20]);
int escribirArchivo(gato*);
int modificar_Archivo(const char[20]);
gato* devolver_gato(char[20]);
int obtener_Rescatados(const char*);
/***********************************Archivos**********************************/

/***********************************Memoria compartida**********************************/
acciones* abrir_mem_comp();
void cerrar_mem_comp(acciones*);
/***********************************Memoria compartida**********************************/

int main(int argc, char *argv[]){
    if(argc > 1){
        if((strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0) && argc == 2){
            Ayuda(argv[1]);
            exit(EXIT_SUCCESS);
        }
        else{
            cout << "Error, el servidor no debe recibir parametros" << endl;
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid, sid;
    int i;

    // Ignora la señal de E / S del terminal, señal de PARADA
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGHUP,SIG_IGN);

    pid = fork();
    if (pid < 0) 
        exit(EXIT_FAILURE); // Finaliza el proceso padre, haciendo que el proceso hijo sea un proceso en segundo plano
    if (pid > 0)
        exit(EXIT_SUCCESS);
    // Cree un nuevo grupo de procesos, en este nuevo grupo de procesos, el proceso secundario se convierte en el primer proceso de este grupo de procesos, de modo que el proceso se separa de todos los terminales    

    sid = setsid();

    if (sid < 0) {
         exit(EXIT_FAILURE);
    }

	// Cree un nuevo proceso hijo nuevamente, salga del proceso padre, asegúrese de que el proceso no sea el líder del proceso y haga que el proceso no pueda abrir una nueva terminal
	pid=fork();
	if( pid > 0) {
		exit(EXIT_SUCCESS);
	}
	else if( pid< 0) {
		exit(EXIT_FAILURE);
	}
  
	// Cierre todos los descriptores de archivos heredados del proceso padre que ya no son necesarios
	for(i=0;i< NOFILE;close(i++));

    // Cambia el directorio de trabajo para que el proceso no contacte con ningún sistema de archivos
    //if ((chdir("/")) < 0) {
      //  exit(EXIT_FAILURE);
    //}    

	// Establece la palabra de protección del archivo en 0 en el momento
	umask(0);

    // el O_CREAT en este caso dice, si no esta crearlo. Si el semaforo no existe, crealo.
    inicializarSemaforos();

    //P(Servidor)
    sem_wait(semaforos[0]);
    /*************************************************************************************************/
    //P(MC)
    sem_wait(semaforos[2]);
    //crear la memoria compartida
    int idMemoria = shm_open(NombreMemoria, O_CREAT | O_RDWR, 0600); // obtenemos un numero que nos identifica esta memoria.
    //me va a determinar/setear el tamaño de la memoria, asociara los tamaños y nos limpiara un poco lo que hay allí dentro.
    ftruncate(idMemoria,sizeof(acciones));

    // definir nuestra variable que es la variable que estara mapeada a memoria compartida.
    // la memoria compartida ya esta creada y tenermos un identificador, pero no podemos accederla..

    //me va a mapear, o a relacionar un segmento de memoria a una variable. agarra un espacio de memoria y mapearlo/darnos la direccion de memoria de ese espacio de memoria.
    acciones *memoria = (acciones*)mmap(NULL, sizeof(acciones), PROT_READ | PROT_WRITE, MAP_SHARED, idMemoria,0);

    close(idMemoria);

    // con esto ya no estara relacionado más a la memoria compartida. Quizas cambiarlo ya que tendremos que cerrarla cuando se envie la señal
    munmap(memoria, sizeof(acciones));
    
    //creamos una memoria compartida especial donde guardaremos el pid de otro proceso que nos servira para matar el servidor mediante la señal sigusR1
    int idAux = shm_open(MemPid, O_CREAT | O_RDWR, 0600);
    ftruncate(idAux,sizeof(int));
    int *pidA = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, idAux,0);
    close(idAux);
    *pidA = getpid();
    munmap(pidA,sizeof(int));


    sem_post(semaforos[2]);
    //V(MC)
    
    //Manejo de señales, cuando se reciban algunas de las dos señales se ejecutara su correspondiente función.
    signal(SIGUSR1, liberar_Recursos);
    //si esta la señal Ctrl+c la ignora.
    signal(SIGINT,SIG_IGN);

    //V(Cliente) // A partir de aqui ya podran operar los clientes.
    sem_post(semaforos[1]);
    //V(TC)
    sem_post(semaforos[3]);

    while (1) {
        sem_wait(semaforos[4]);
        // P (TS) Turno del servidor, inicia el 0 y solo tendra un 1 de valor una ves que termine de ejecutar algun proceso cliente.
        sem_wait(semaforos[2]);
        // P(MC) Bloqueamos la memoria compartida.
        realizar_Actividades();
        sem_post(semaforos[2]);
        //V(MC)
        sem_post(semaforos[3]);
        // V(TC)
    }
   exit(EXIT_SUCCESS);
}

void realizar_Actividades(){
        acciones *memoria = abrir_mem_comp();
        if(memoria->alta == 1){
            gato *aux = (gato*)malloc(sizeof(gato));
            strcpy(aux->situacion,memoria->g.situacion);
            strcpy(aux->nombre,memoria->g.nombre);
            strcpy(aux->raza,memoria->g.raza);
            strcpy(aux->estado,memoria->g.estado);
            strcpy(aux->sexo,memoria->g.sexo);
            if(escribirArchivo(aux) == -1){
                strcpy(memoria->notialta,"El nombre ya existe, pruebe con otro");
            }
            strcpy(memoria->g.situacion,"");
            strcpy(memoria->g.nombre,"");
            strcpy(memoria->g.raza,"");
            strcpy(memoria->g.sexo,"");
            strcpy(memoria->g.estado,"");
            free(aux);
            memoria->alta = 0;
        }
        if(memoria->baja == 1){
            int res = modificar_Archivo(memoria->g.nombre);
            if(res == -1){
                strcpy(memoria->notibaja,"El gato no se encuentra registrado, vuelva a intentarlo");
            }
            if(res == -2){
                strcpy(memoria->notibaja,"El gato ya estaba de baja");
            }
            strcpy(memoria->g.nombre,"");
            memoria->baja = 0;
        }
        if(memoria->consultar == 1){
            if(strcmp(memoria->g.nombre,"") != 0){ //significa que se ingreso el nombre
                int pos = consultarArchivo(memoria->g.nombre);
                if(pos == -1 || pos == -2)
                    strcpy(memoria->consulta,"El gato no se encuentra registrado");
                else{ //si el gato si fue encontrado...
                    gato *aux = devolver_gato(memoria->g.nombre);
                    strcpy(memoria->g.situacion,aux->situacion);
                    strcpy(memoria->g.raza,aux->raza);
                    strcpy(memoria->g.sexo,aux->sexo);
                    strcpy(memoria->g.estado,aux->estado);
                    strcpy(memoria->consulta,"");
                    free(aux);
                }
            }
            else{ //mostrar toda la tabla de TODOS LOS GATOS RESCATADOS, es decir, los que poseen ALTA
                int res = obtener_Rescatados(memoria->rescatados);
                if(res == -2 || res == -1)
                    strcpy(memoria->consulta,"No se encuentran gatos rescatados (situacion = ALTA)");
            }
            memoria->consultar = 0;
        }
        cerrar_mem_comp(memoria);
}


void eliminar_Sem(){
    sem_close(semaforos[0]);
    sem_close(semaforos[1]);
    sem_close(semaforos[2]);
    sem_close(semaforos[3]);
    sem_close(semaforos[4]);
    sem_unlink("servidor");
    sem_unlink("cliente");
    sem_unlink("memComp");
    sem_unlink("t_Cliente");
    sem_unlink("t_Servidor");
}

void inicializarSemaforos(){
    semaforos[0] = sem_open("servidor",O_CREAT,0600,1);
    
    // Si dicho semaforo vale 0 en ese momento significa que ya hay otra instancia de semaforo ejecutando por lo que cerramos el proceso.
    int valorSemServi = 85;
    sem_getvalue(semaforos[0],&valorSemServi);
    if(valorSemServi == 0)
        exit(EXIT_FAILURE);
    semaforos[1] = sem_open("cliente",O_CREAT,0600,0);
    semaforos[2] = sem_open("memComp",O_CREAT,0600,1);
    semaforos[3] = sem_open("t_Cliente",O_CREAT,0600,0);
    semaforos[4] = sem_open("t_Servidor",O_CREAT,0600,0);
}

void liberar_Recursos(int signum){
    // Si dicho semaforo vale 0 en ese momento significa que ya hay otra instancia de semaforo ejecutando por lo que cerramos el proceso.
    int valorTurnoClie = 22;
    sem_getvalue(semaforos[3],&valorTurnoClie);
    if(valorTurnoClie == 0){
        int valorMC = 22;
        sem_getvalue(semaforos[2],&valorMC);
        if(valorMC == 0){
            //en este punto la señal llego cuando el servidor justo había abierto la memoria para atender al cliente
            realizar_Actividades();
            sem_post(semaforos[2]);
        }
        else{
            sem_wait(semaforos[2]);
            realizar_Actividades();
            sem_post(semaforos[2]);
        }
        sem_post(semaforos[1]);
        sem_post(semaforos[3]);
        // Esperamos a que el cliente termine...
        sleep(2);
    }
    eliminar_Sem();
    shm_unlink("miMmemoria");
    shm_unlink("pidServidor");
    remove("gatos.txt");
    remove("salida.txt");
    exit(EXIT_SUCCESS);
}

bool Ayuda(const char *cad)
{
    if (!strcmp(cad, "-h") || !strcmp(cad, "--help") )
    {
        cout << "Esta script quedara ejecutando en segundo plano como demonio." << endl;
        cout << "La cual dara servicio a otra script llamada Cliente." << endl;
        cout << "Dependiendo de lo que desee el cliente, este proceso realizara las correspondientes acciones" << endl;
        cout << "Alta, registra de poder, al gato en cuestion en el archivo." << endl;
        cout << "Baja, si dicho gato fue adoptado, modifica el estado de dicho gato en el archivo" << endl;
        cout << "Consulta, traera algún gato particular o listara todos los gatos rescatados." << endl;
        cout << "solo se ejecuta de la siguiente manera ./Servidor" << endl;
        cout << "Para finalizar el proceso servidor simplemente basta con ejecutar ./Disparador" << endl;
        return true;
    }
    return false;
}

int consultarArchivo(const char nombre[20]){
    ifstream archivo;
    string texto;
    char gatito[60];
    char *pch;
    archivo.open("gatos.txt",ios::in);
    if(archivo.fail()){
        cout << "no se pudo abrir el archivo" << endl;
        return -2;
    }
    int cont = 0;

    while(getline(archivo,texto)){
        strcpy(gatito,texto.c_str());
        if(strcmp(gatito,"") == 0)
            break;
        //aqui obtenemos la situacion del gato
        pch = strtok(gatito, "|");
        //aqui obtenemos el nombre del gato
        pch = strtok(NULL, "|");
        if(strcmp(nombre,pch) == 0){
            archivo.close();
            return cont;
        }
        cont++;
    }
    archivo.close();
    return -1;
}

int escribirArchivo(gato *g){
    if(consultarArchivo(g->nombre) >= 0){
        return -1;  //si ya esta, se debe escribir el mensaje en memoria->notialta que ya esta el nombre usado y este es único.
    }
    ofstream archivo;
    archivo.open("gatos.txt",ios::app);
    if(archivo.fail()){
        cout << "no se pudo abrir el archivo" << endl;
        exit(1);
    }
    string tmp_situacion(g->situacion);
    string tmp_nombre(g->nombre);
    string tmp_raza(g->raza);
    string tmp_sexo(g->sexo);
    string tmp_estado(g->estado);
    archivo << tmp_situacion + "|" + tmp_nombre + "|" + tmp_raza + "|" + tmp_sexo + "|" + tmp_estado << endl;
    archivo.close();
    return 1;
}

int modificar_Archivo(const char nombre[20]){
    int pos = consultarArchivo(nombre);
    string texto;
    char gatito[60];
    char *pch;
    if(pos < 0){
        return -1;  //si no existe, se debe escribir el mensaje en memoria->notibaja que el nombre del gato no se encuentra registrado.
    }
    ifstream archivo;
    archivo.open("gatos.txt",ios::in);
    if(archivo.fail())
        exit(1);
    //cambiamos el ALTA, por BAJA en dicho gato
    ofstream auxiliar;
    auxiliar.open("auxiliar.txt",ios::out);
    if(auxiliar.fail()){
        archivo.close();
        exit(1);
    }
    int cont = 0;
    while(getline(archivo,texto)){
        strcpy(gatito,texto.c_str());
        if(strcmp(gatito,"") == 0)
            break;
        //aqui obtenemos la situacion del gato
        if(pos == cont){    // es el gato a cambiar la situacion de ALTA a BAJA
            char *situacionvieja = strtok(gatito,"|");
            if(strcmp(situacionvieja,"BAJA") == 0){
                archivo.close();
                auxiliar.close();
                remove("auxiliar.txt");
                return -2;
            }
            else{
                char *nombregato = strtok(NULL,"|");
                char *raza = strtok(NULL,"|");
                char *sexo = strtok(NULL,"|");
                char *estado = strtok(NULL,"|");
                string tmp_nombregato(nombregato);
                string tmp_raza(raza);
                string tmp_sexo(sexo);
                string tmp_estado(estado);
                auxiliar << "BAJA|" + tmp_nombregato+ "|" + tmp_raza + "|" + tmp_sexo + "|" + tmp_estado << endl;
            }
        }
        else{
            auxiliar << texto << endl;
        }
        pch = strtok(gatito, "|");
        //aqui obtenemos el nombre del gato
        pch = strtok(NULL, "|");
        cont++;
    }
    archivo.close();
    auxiliar.close();
    remove("gatos.txt");
    rename("auxiliar.txt","gatos.txt");
    return 0;
}

gato* devolver_gato(char nombre[20]){
    gato *g = (gato*) malloc(sizeof(gato));
    ifstream archivo;
    string texto;
    char gatito[60];
    char *pch;
    char situacion[5];
    archivo.open("gatos.txt",ios::in);
    if(archivo.fail()){
        cout << "no se pudo abrir el archivo" << endl;
        exit(1);
    }

    while(getline(archivo,texto)){
        strcpy(gatito,texto.c_str());
        //aqui obtenemos la situacion del gato
        pch = strtok(gatito, "|");
        //aqui obtenemos el nombre del gato
        strcpy(situacion,pch);
        pch = strtok(NULL, "|");
        if(strcmp(nombre,pch) == 0){
            strcpy(g->situacion,situacion);
            strcpy(g->nombre,pch);
            pch = strtok(NULL, "|");
            strcpy(g->raza, pch);
            pch = strtok(NULL, "|");
            strcpy(g->sexo,pch);
            pch = strtok(NULL, "|");
            strcpy(g->estado, pch);
            archivo.close();
            return g;
        }
    }

    archivo.close();

    return NULL;
}

int obtener_Rescatados(const char *path){
    ifstream archivo1;
    ofstream archivo2;
    string texto;
    char gatito[100];
    char *pch;
    archivo1.open("gatos.txt",ios::in);
    if(archivo1.fail())
        return -1;
    string tempString(path);
    archivo2.open(tempString,ios::out);
    if(archivo2.fail()){
        archivo1.close();
        return -2;
    }
    while(getline(archivo1,texto)){
        strcpy(gatito,texto.c_str());
        pch = strtok(gatito, "|");
        if(strcmp(pch,"ALTA") == 0) {     //consideramos a los gatos en situación de ALTA como rescatados.
            archivo2 << texto << endl;
        }
    }
    archivo1.close();
    archivo2.close();
    return 0;
}

acciones* abrir_mem_comp(){
    int idMemoria = shm_open(NombreMemoria, O_CREAT | O_RDWR, 0600); // obtenemos un numero que nos identifica esta memoria.
    // definir nuestra variable que es la variable que estara mapeada a memoria compartida.
    // la memoria compartida ya esta creada y tenermos un identificador, pero no podemos accederla..
    //me va a determinar/setear el tamaño de la memoria, asociara los tamaños y nos limpiara un poco lo que hay allí dentro.
    //ftruncate(idMemoria,sizeof(acciones));

    //me va a mapear, o a relacionar un segmento de memoria a una variable. agarra un espacio de memoria y mapearlo/darnos la direccion de memoria de ese espacio de memoria.
    acciones *memoria = (acciones *)mmap(NULL, sizeof(acciones), PROT_READ | PROT_WRITE, MAP_SHARED, idMemoria,0);

    close(idMemoria);
    return memoria;
}

void cerrar_mem_comp(acciones *a){
    munmap(a, sizeof(acciones));
}