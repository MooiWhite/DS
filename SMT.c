#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define SMOBJ_NAME "/ObjSharedMem"
#define SMOBJ_SIZE 400
#define NUM_T      8
//const int NUM_T = 5;

static int counter = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void create_OSM(void){
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
}
void write_OSM(int c){
    int fd;
    char buf[]= "Added";
    char *ptr;

    sprintf(buf+strlen(buf),"%d",c);
    printf("Writing :%s\n",buf);
    //printf("Size buuf is: %d\n", sizeof(buf));
    fd = shm_open(SMOBJ_NAME, O_RDWR, 0); //Abrimos un objeto de lectura compartido
    if (fd == -1){
        printf("error, shared memory could not be readed \n");
        exit(1);
    }
    ptr =  mmap(0, sizeof(buf), PROT_WRITE, MAP_SHARED,fd, 0);
    
    if (ptr == MAP_FAILED){
        printf("Error in memory mapping\n");
        exit(1);
    }
    memcpy(ptr,buf,sizeof(buf));
}

void read_OSM(void){
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
    printf("Reading: %s \n", ptr);
    close(fd);
}


void *thread1_routine(void *unused){
    //pthread_mutex_lock(&mutex);
    counter++;
    write_OSM(counter);
    //pthread_mutex_unlock(&mutex);

}

void *thread2_routine(void *unused){
    //pthread_mutex_lock(&mutex);
    read_OSM();
    //pthread_mutex_unlock(&mutex);
}

void errorExit(char *strerr){
    perror(strerr);
    exit(1);
}

int main(void){
    //pthread_t thread1, thread2;
    pthread_t threads [NUM_T];

    create_OSM(); //Creating shared memory object

    for (int i = 0; i < NUM_T-4; i++){
        if (0 != pthread_create(&threads[i],NULL, thread1_routine,NULL))
        errorExit("thread writing connot be created");
    }   

    for (int i = NUM_T-4; i < NUM_T; i++){
        if (0 != pthread_create(&threads[i],NULL, thread2_routine,NULL))
        errorExit("thread reading cannot be created");
    } 

    for (int i = 0; i < NUM_T; i++){
        pthread_join(threads[i], NULL);
    }

    return 0;
}
