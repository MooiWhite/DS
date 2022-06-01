#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <uuid/uuid.h>
#include <errno.h>
#include <malloc.h>

//#define SMOBJ_NAME "/myMemoryObj"
#define FILENAME "test.txt"
#define SMOBJ_SIZE 400
#define LEVEL_REP 2
//#define CHUNK_SIZE 10
#define NUM_CH_SERVERS 4

#define NUM_T 8

typedef struct Master {
    char *filename;
    unsigned char IDChunk;
    size_t *IDChunkServer;
}Master;

int insert_on_master(char* name,unsigned char IDChunk,size_t *IDserver,Master* master){
    master->filename = name;
    master->IDChunk = IDChunk;
    master->IDChunkServer = IDserver;
    return 0;
}

int init_chunkserver(char* ID){
    int fd;
    fd = shm_open(ID, O_CREAT | O_RDWR, 00600); //Creamos un objeto de memoria compartido
    if (fd == -1){
        printf("error, shared memory could not be created \n");
        exit(1);
    }
    if ( 1 == ftruncate(fd,SMOBJ_SIZE)){
        printf("error, shared could not be sized\n");
        exit(1);
    }
    return fd;
}

void write_chunkserver(int fd, char* buf){
    printf("Into chunk I received this: %s , Size: %u\n",buf,strlen(buf));
    char *ptr;
    ptr =  mmap(0, strlen(buf), PROT_WRITE, MAP_SHARED,fd, 0);
    if (ptr == MAP_FAILED){
        printf("Error in memory mapping\n");
        exit(1);
    }
    memcpy(ptr,buf,strlen(buf));
}

void replication(int ID, char* buf){
    int fd;
    char *ptr;
    char key[2];
    
    for (int i=0; i<LEVEL_REP;i++){
       // if (i != ID/*LEVEL_REP*/){ //OR ID?
            sprintf(key,"%d",i); //Convert int to string and put the result in ID
            fd = shm_open(key, O_RDWR, 0); //Creamos un objeto de memoria compartido
            printf("%s,Level: %d END | ID: %d\n",buf,LEVEL_REP,ID);
            ptr =  mmap(0, strlen(buf), PROT_WRITE, MAP_SHARED,fd, 0);
            if (ptr == MAP_FAILED){
                printf("Error in memory mapping\n");
                exit(1);
            }
            strncat(ptr,buf,strlen(buf));
            close(fd);
        //}
    }
}

size_t* save_server_index(size_t level_rep){
    //printf("Antes de la tragedia;\n");
    //printf("LEVEL_REP:%d\n",level_rep);
    size_t *array_index = malloc(level_rep*sizeof(level_rep)); 
    //printf("Después de la tragedia;\n");
    for (size_t i=0; i<level_rep;i++){
        array_index[i] = i;
        //printf("%d\n",array_index[i]);
    }
    //printf("LENGHT: %d",sizeof(array_index));
    return array_index;
}

int open_file(char* filename, size_t chunk_size, Master* master, size_t offset){
    char ID[10];
    char ID_CHUNK[2];
    int fd;
    char buf[chunk_size];
    uuid_t binuuid;

    FILE* ptr = fopen(filename, "r");
        if (ptr == NULL) {
            printf("no such file");
            return 0;
        }  

    printf("CHUNKSIZE: %d\n",chunk_size);
    //Conversion file to chunks on chunkservers

    //Initialize chunkservers
    for (int i=0;i<NUM_CH_SERVERS;i++){
        fgets(buf, chunk_size+1, ptr);//Read a chunk of a file
        sprintf(ID,"%d",i); //Convert int to string and put the result in ID
        fd = init_chunkserver(ID); //Init a chunkserver with its ID
        write_chunkserver(fd,buf); //Write on an specific chunkserver with fd
    }

    ptr = fopen(filename, "r");

    printf("\nSTARTING REPLICATION: \n");
    //Replication on chunkservers
    for (int i=0; i<offset; i++){

        fgets(buf, chunk_size+1, ptr);//Read a chunk of a file
        replication(i,buf);

        for (int j=0;j<LEVEL_REP;j++){
            uuid_generate_random(binuuid);
            insert_on_master(FILENAME,*binuuid,save_server_index(LEVEL_REP),&master[i]);
        }    
    }
}

void *thread1_routine(void *unused){
    //pthread_mutex_lock(&mutex);
    //counter++;
    //write_OSM(counter);
    //pthread_mutex_unlock(&mutex);

}

void *thread2_routine(void *unused){
    //pthread_mutex_lock(&mutex);
    //read_OSM();
    //pthread_mutex_unlock(&mutex);
}

void errorExit(char *strerr){
    perror(strerr);
    exit(1);
}

int main(void){
    char ID[10];
    int fd;
    size_t offset;
    size_t CHUNK_SIZE;
    struct stat shmobj_st; //for get the offset
    stat (FILENAME, &shmobj_st);
    Master master[10];//We will have a certain number of rows as files we have
    
    CHUNK_SIZE = shmobj_st.st_size / NUM_CH_SERVERS; //Number of bytes that every ch_server will have  
    offset = shmobj_st.st_size / CHUNK_SIZE; //NUmber of of chunks that totally we will have == Level of replication IT DEPENDS

    
    open_file(FILENAME, CHUNK_SIZE, master,offset);

    printf("\nPRINTING TABLE MASTER\n");
    //It prints rows [number of chunks divided] times
    for (int i=0; i<offset; i++){
        printf("Row master test [FILENAME]: %s\n",master[i].filename);
        printf("Row master test [IDChunk]: %d\n",master[i].IDChunk);
        printf("Row master test [INDEXES CHUNK]:");
        for (size_t j = 0; j<LEVEL_REP; j++){
            printf("%d,", master[i].IDChunkServer[j]);
        }
        printf("\n\n");
    }

//LECTURA

    /*int filed;
    char *pointer;*/
    pthread_t threads [NUM_T];

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

    /*filed = shm_open("2", O_RDONLY, 0); //Creamos un objeto de memoria compartido
    if (filed == -1){
        printf("Error file descriptor %s\n",strerror(errno));
        exit(1);
    }

    if (fstat(filed,&shmobj_st) == -1){
        printf("Error fstat\n");
        exit(1);
    }

    pointer =  mmap(NULL, 2, PROT_READ, MAP_SHARED,filed, 0);
    if (pointer == MAP_FAILED){
        printf("Mapp failed in read mapping process\n");
        exit(1);
    }

    printf("%s \n", pointer);
    close(filed);*/

    return 0;
}
