#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SMOBJ_NAME "/myMemoryObj"
#define SMOBJ_SIZE 30

int main(void){
    int fd;
    fd = shm_open(SMOBJ_NAME, O_CREAT | O_RDWR, 00600); //Creamos un objeto de memoria compartido
    if (fd == -1){
        printf("error, shared memory could not be created \n");
        exit(1);
    }
    if ( 1 == ftruncate(fd,SMOBJ_SIZE)){
        printf("error, shared could not be sized\n");
        exit(1);
    }
    return 0;
}

