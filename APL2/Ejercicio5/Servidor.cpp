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
#include <string>
#include <string.h>
#include <sys/types.h>
#include <linux/fs.h>
#include <sys/param.h>
#include <time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

typedef struct {
    char situacion[5]; // ALTA (ingreso/rescatado) BAJA (adopcion/egreso)
    char nombre[21];
    char raza[21];
    char sexo[2];    // M o H
    char estado[3]; // CA (castrado) o SC (sin castrar) 
}gato;

typedef struct {
    int pidServ;
    int Socket_Escucha;
}dato;

sem_t* semaforos[1];
/*
    0 - Servidor (solo puede haber 1) se inicia en 1 y rapidamente se ocupa por el primer demonio. se liberara cuando el servidor demonio sea detenido
*/

using namespace std;
//necesitamos un identificador para la memoria compartida para que los diferentes procesos que vayan a utilizarla tengan una manera de referenciarla
#define MemPid "pidServidorSocket"
#define SERV_HOST_ADDR "127.0.0.1"     /* IP, only IPV4 support  */

string realizar_Actividades(const char[]);
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
gato* devolver_gato(const char[20]);
int obtener_Rescatados(const char*);
/***********************************Archivos**********************************/


int main(int argc, char *argv[]){
    if(argc > 1){
        if((strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0) && argc == 2){
            Ayuda(argv[1]);
            exit(EXIT_SUCCESS);
        }
        else{
            if((strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0) && argc > 2){
                cout << "Error, el servidor no debe recibir parametros junto con el de ayuda" << endl;
                exit(EXIT_FAILURE);
            }
            else{
                /*
                if((strcmp(argv[1],"-h") != 0 && strcmp(argv[1],"--help") != 0) && argc > 2){
                    cout << "Error, el servidor no debe recibir parametros junto con el host" << endl;
                    exit(EXIT_FAILURE);
                }
                */
                if((strcmp(argv[1],"-h") != 0 && strcmp(argv[1],"--help") != 0) && argc != 3){
                    cout << "Error, el servidor debe recibir la ip y el host del servidor." << endl;
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    else{
        cout << "Error, cantidad de parametros erroneo, para ayuda ejecute ./Servidor -h o ./Servidor --help" << endl;
        exit(EXIT_FAILURE);
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

    //Manejo de señales, cuando se reciban algunas de las dos señales se ejecutara su correspondiente función.
    signal(SIGUSR1, liberar_Recursos);
    //si esta la señal Ctrl+c la ignora.
    signal(SIGINT,SIG_IGN);

    ofstream archivo;
    archivo.open("gatos.txt",ios::out);
    if(archivo.fail())
        exit(EXIT_FAILURE);
    archivo << "Situacion|Nombre|Raza|Sexo|Estado";
    archivo.close();
    struct sockaddr_in serverConfig;
    memset(&serverConfig,'0',sizeof(serverConfig));

    serverConfig.sin_family = AF_INET; //IPV4: 127.0.0.1
    //Direcciones desde la cuales estamos esperando conexiones.
    //INADDR_ANY, estoy escuchando conexiones desde cualquier IP
    //serverConfig.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR); 
    //serverConfig.sin_addr.s_addr = htonl(INADDR_ANY);

    //en caso de pasar la ip del servidor descomentar esto.
    serverConfig.sin_addr.s_addr = inet_addr(argv[1]);


    //le pasamos el puerto, en este caso el 5000
    //serverConfig.sin_port = htons(atoi(argv[1]));
    
    //en caso de pasar la ip del servidor descomentar esto.
    serverConfig.sin_port = htons(atoi(argv[2]));

    //htonl y htons son porque la estructura de configuración de los sockets utiliza un formato distinto en la manera en la que se guardan los numeros que se estan usando en C.
    //estas funciones convierten esos numeros a un modo compatible con los sockets.

    //Creamos el socket
    //AF_INET: que sera IPV4
    //SOCK_STREAM: esteremos usando el protocolo tcp. 
    int socketEscucha = socket(AF_INET,SOCK_STREAM,0);
    //nos va a linkear/relacionar nuestro socket con nuestra configuración.
    //bind asinga una direccion ip y un puerto al socket.
    bind(socketEscucha,(struct sockaddr *)&serverConfig,sizeof(serverConfig));

    listen(socketEscucha,3); // hasta 3 clientes pueden estar encolados, hace que el socket se vuelva pasivo. Es decir, quede escuchando.

    //creamos una memoria compartida especial donde guardaremos el pid del servidorSocket y el socket de escucha.
    int idAux = shm_open(MemPid, O_CREAT | O_RDWR, 0600);
    ftruncate(idAux,sizeof(dato));
    dato *pidA = (dato*)mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, idAux,0);
    close(idAux);
    pidA->pidServ = getpid();
    pidA->Socket_Escucha = socketEscucha;
    munmap(pidA,sizeof(dato));
    //
    while (1) {
        int socketComunicacion = accept(socketEscucha, (struct sockaddr *) NULL, NULL);
        char mensajeCliente[2000];
        bzero(mensajeCliente,2000);
        int bytesRecibidos = 0;
        bytesRecibidos = read(socketComunicacion,mensajeCliente,sizeof(mensajeCliente) - 1);
        if(bytesRecibidos > 0){   
            string sendBuff = realizar_Actividades(mensajeCliente);
            //Escribimos en el socket de comunicacion que vamos a mandar y el tamaño que tiene lo que vamos a mandar
            char respuestaServidor[2000];
            bzero(respuestaServidor,2000);
            strcpy(respuestaServidor,"");
            strcpy(respuestaServidor,sendBuff.c_str());
            //char cad[] = "Hola! Soy el proceso servidor";
            //strcpy(cad,"");
            //strcpy(cad,mensajeCliente);
            write(socketComunicacion,respuestaServidor,strlen(respuestaServidor));
            close(socketComunicacion);
        }
    }
   exit(EXIT_SUCCESS);
}

string realizar_Actividades(const char mensaje[]){
    char aux[2000];
    strcpy(aux,mensaje);
    char *p = strtok(aux,"|");
    if(strcmp(p,"ALTA") == 0){
        gato *g = (gato*)malloc(sizeof(gato));
        if(g == NULL)
            exit(EXIT_FAILURE);
        strcpy(g->situacion,p);
        p = strtok(NULL,"|");
        strcpy(g->nombre,p);
        p = strtok(NULL,"|");
        strcpy(g->raza,p);
        p = strtok(NULL,"|");
        strcpy(g->sexo,p);
        p = strtok(NULL,"|");
        strcpy(g->estado,p);
        int i = escribirArchivo(g);
        free(g);
        if(i == -1)
            return "Error, el gato ya se encuentra registrado";
        if(i == -2)
            return "Error, el archivo no pudo abrirse en modo escritura";
        return "Operacion exitosa";
    }
    else{
        if(strcmp(p,"BAJA") == 0){
            char *ptr_nombre = strtok(NULL,"|");
            int r = modificar_Archivo(ptr_nombre);
            if(r == -2)
                return "Error, el gato ya estaba dado de baja";
            if(r == -1)
                return "Error, el gato no se encuentra registrado";
            return "Operacion exitosa";
        }
        else{
            if(strcmp(p,"CONSULTA") == 0){
                // La accion a realizar es consultar en este caso.
                char *ptr_nombre = strtok(NULL,"|");
                if(strcmp(ptr_nombre,"rescatados.txt") == 0){
                    int r = obtener_Rescatados(ptr_nombre);
                    if(r == 0)
                        return "No se hallan gatos rescatados";
                    else
                        return "Operacion exitosa";
                }
                else{
                    gato *g = devolver_gato(ptr_nombre);
                    if(g == NULL)
                        return "Error, el gato no se encuentra registrado";
                    char respuesta[2000];
                    bzero(respuesta,2000);
                    strcat(respuesta,g->situacion);
                    strcat(respuesta,"|");
                    strcat(respuesta,g->nombre);
                    strcat(respuesta,"|");
                    strcat(respuesta,g->raza);
                    strcat(respuesta,"|");
                    strcat(respuesta,g->sexo);
                    strcat(respuesta,"|");
                    strcat(respuesta,g->estado);
                    //strcat(respuesta,"\0");
                    free(g);
                    string res(respuesta);
                    return res;
                }
            }
        }
    }
    return NULL;
}


void eliminar_Sem(){
    sem_close(semaforos[0]);
    sem_unlink("servidorSocket");
}

void inicializarSemaforos(){
    semaforos[0] = sem_open("servidorSocket",O_CREAT,0600,1);
    
    // Si dicho semaforo vale 0 en ese momento significa que ya hay otra instancia de semaforo ejecutando por lo que cerramos el proceso.
    int valorSemServi = 85;
    sem_getvalue(semaforos[0],&valorSemServi);
    if(valorSemServi == 0)
        exit(EXIT_FAILURE);
}

void liberar_Recursos(int signum){
    eliminar_Sem();
    //creamos una memoria compartida especial donde guardaremos el pid del servidorSocket y el socket de escucha.
    int idAux = shm_open(MemPid, O_CREAT | O_RDWR, 0600);
    dato *pidA = (dato*)mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, idAux,0);
    close(idAux);
    shutdown(pidA->Socket_Escucha,SHUT_RDWR);
    munmap(pidA,sizeof(dato));
    shm_unlink("pidServidorSocket");
    //
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
        cout << "solo se ejecuta de la siguiente manera ./Servidor [IP servidor] [host servidor]" << endl;
        cout << "Para finalizar el proceso servidor simplemente basta con ejecutar ./Disparador" << endl;
        return true;
    }
    return false;
}

int consultarArchivo(const char nombre[21]){
    ifstream archivo("gatos.txt");
    if(!archivo.is_open()){
        return -2;
    }
    string texto;
    char gatito[100];
    char *pch;
    int cont = 1;
    getline(archivo,texto);
    while(getline(archivo,texto)){
        strcpy(gatito,texto.c_str());
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
        return -1;
    }
    ofstream archivo;
    archivo.open("gatos.txt",ios::app);
    if(archivo.fail()){
        return -2;
    }
    string tmp_situacion(g->situacion);
    string tmp_nombre(g->nombre);
    string tmp_raza(g->raza);
    string tmp_sexo(g->sexo);
    string tmp_estado(g->estado);
    archivo << "\n" + tmp_situacion + "|" + tmp_nombre + "|" + tmp_raza + "|" + tmp_sexo + "|" + tmp_estado;
    archivo.close();
    return 1;
}

int modificar_Archivo(const char nombre[21]){
    int pos = consultarArchivo(nombre);
    string texto;
    char gatito[60];
    char *pch;
    if(pos == -1 || pos == -2){
        return -1;
    }
    ifstream archivo;
    archivo.open("gatos.txt",ios::in);
    if(archivo.fail()){
        return -1;
    }
    //cambiamos el ALTA, por BAJA en dicho gato
    ofstream auxiliar;
    auxiliar.open("auxiliar.txt",ios::out);
    if(auxiliar.fail()){
        archivo.close();
        return -1;
    }
    int cont = 1;
    getline(archivo,texto);
    auxiliar << texto;
    while(getline(archivo,texto)){
        strcpy(gatito,texto.c_str());
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
                auxiliar << "\nBAJA|" + tmp_nombregato+ "|" + tmp_raza + "|" + tmp_sexo + "|" + tmp_estado;
            }
        }
        else{
            auxiliar << "\n" + texto;
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

gato* devolver_gato(const char nombre[21]){
    gato *g = (gato*) malloc(sizeof(gato));
    ifstream archivo;
    string texto;
    char gatito[100];
    char *pch;
    char situacion[5];
    archivo.open("gatos.txt",ios::in);
    if(archivo.fail()){
        return NULL;
    }
    getline(archivo,texto);
    while(getline(archivo,texto)){
        strcpy(gatito,texto.c_str());
        //aqui obtenemos la situacion del gato
        pch = strtok(gatito, "|");
        strcpy(situacion,pch);
        //aqui obtenemos el nombre del gato
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
    free(g);
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
    int cont = 0;
    while(getline(archivo1,texto)){
        strcpy(gatito,texto.c_str());
        if(strcmp(gatito,"") == 0)
            break;
        pch = strtok(gatito, "|");
        if(strcmp(pch,"ALTA") == 0) {     //consideramos a los gatos en situación de ALTA como rescatados.
            archivo2 << texto << endl;
            cont++;
        }
    }
    archivo1.close();
    archivo2.close();
    return cont;
}