#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#define SMOBJ_NAME "/myMemoryObj"

int main(void){
    int fd;
    char buf[]= "Hi this is a simple and normal text\n";
    char *ptr;

    fd = shm_open(SMOBJ_NAME, O_RDWR, 0); //Creamos un objeto de memoria compartido
    if (fd == -1){
        printf("error, shared memory could not be readed \n");
        exit(1);
    }
    ptr =  mmap(0, sizeof(buf), PROT_WRITE, MAP_SHARED,fd, 0);
    printf("Size: %d\n",sizeof(buf));
    if (ptr == MAP_FAILED){
        printf("Error in memory mapping\n");
        exit(1);
    }
    memcpy(ptr,buf,sizeof(buf));
    return 0;
}
