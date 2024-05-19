#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <linux/fs.h>
#include <sys/param.h>
#include <semaphore.h>
using namespace std;

typedef struct {
    int pidServ;
    int Socket_Escucha;
}dato;

#define MemPid "pidServidorSocket"

int main(){
    int idAux = shm_open(MemPid, 0100 | 02, 0600);
    dato *pidA = (dato*)mmap(NULL, sizeof(dato), PROT_READ | PROT_WRITE, MAP_SHARED, idAux,0);
    close(idAux);
    int pid = pidA->pidServ;
    munmap(pidA,sizeof(dato));
    kill(pid,SIGUSR1);
}