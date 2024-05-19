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

#define MemPid "pidServidor"

int main(){
    int idAux = shm_open(MemPid, 0100 | 02, 0600);
    int *pidA = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, idAux,0);
    close(idAux);
    int pid = *pidA;
    munmap(pidA,sizeof(int));
    kill(pid,SIGUSR1);
}