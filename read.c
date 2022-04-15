#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define SMOBJ_NAME "/myMemoryObj"

int main(void){
    int fd;
    char *ptr;
    struct stat shmobj_st;

    fd = shm_open(SMOBJ_NAME, O_RDONLY, 0); //Creamos un objeto de memoria compartido
    if (fd == -1){
        printf("Error file descriptor %s\n",strerror(errno));
        exit(1);
    }

    if (fstat(fd,&shmobj_st) == -1){
        printf("Error fstat\n");
        exit(1);
    }

    ptr =  mmap(NULL, shmobj_st.st_size, PROT_READ, MAP_SHARED,fd, 0);
    if (ptr == MAP_FAILED){
        printf("Mapp failed in read mapping process\n");
        exit(1);
    }
    printf("%s \n", ptr);
    close(fd);
    return 0;
}
