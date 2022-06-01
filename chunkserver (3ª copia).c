#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <uuid/uuid.h>
#include <errno.h>
#include <malloc.h>

#define SMOBJ_NAME "/myMemoryObj"
#define FILENAME "test.txt"
#define SMOBJ_SIZE 400
#define LEVEL_REP 3
//#define CHUNK_SIZE 10
#define NUM_CH_SERVERS 4

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
        if (i != ID/*LEVEL_REP*/){ //OR ID?
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
        }
    }
}

size_t* save_server_index(size_t num_ch_servers){
    printf("Antes de la tragedia;\n");
    size_t *array_index = malloc(num_ch_servers*sizeof(num_ch_servers)); 
    printf("Después de la tragedia;\n");
    for (size_t i=0; i<sizeof(num_ch_servers);i++){
        array_index[i] = i;
    }
    return array_index;
}

int open_file(char* filename, size_t chunk_size){
    char ID[10];
    char ID_CHUNK[2];
    int fd;
    char buf[chunk_size];
    
    uuid_t binuuid;

    //struct stat st; //For get the offset
    //stat (filename, &st);

    FILE* ptr = fopen(filename, "r");
        if (ptr == NULL) {
            printf("no such file");
            return 0;
        }  

    //CHUNK_SIZE = st.st_size / NUM_CH_SERVERS;    
    //offset = st.st_size / CHUNK_SIZE;
 
    //printf("OFFFFFSETTTTT%d\n", offset);
    printf("CHUNKSIZE: %d\n",chunk_size);
    //printf("SIZE: %ld\n",st.st_size);
    
    //Conversion file to chunks on chunkservers

    //Initialize chunkservers
    for (int i=0;i<NUM_CH_SERVERS;i++){
        fgets(buf, chunk_size+1, ptr);//Read a chunk of a file
        //printf("BUFFFERRR: %s\n",buf);
        sprintf(ID,"%d",i); //Convert int to string and put the result in ID
        fd = init_chunkserver(ID); //Init a chunkserver with its ID
        write_chunkserver(fd,buf); //Write on an specific chunkserver with fd
    }

    ptr = fopen(filename, "r");

    //int arrayTest[] = {0,1};
    //char *p;
    Master master[10];//We will have a certain number of rows as files we have

    //Replication on chunkservers
    for (int i=0; i<NUM_CH_SERVERS; i++){

        fgets(buf, chunk_size+1, ptr);//Read a chunk of a file
        replication(i,buf);

        uuid_generate_random(binuuid);
        insert_on_master(FILENAME,*binuuid,save_server_index(NUM_CH_SERVERS),&master[i]);
        printf("Row master test [FILENAME]: %s\n",master[i].filename);
        printf("Row master test [IDChunk]: %d\n",master[i].IDChunk);
        printf("Row master test [INDEXES CHUNK]:");
        for (size_t j = 0; j<sizeof(master[i].IDChunkServer); j++){
            printf("%d,",master[i].IDChunkServer[j]);
        }
        printf("\n");
    }
}


int main(void){
    char ID[10];
    int fd;
    size_t offset;
    size_t CHUNK_SIZE;
    struct stat shmobj_st; //for get the offset
    stat (FILENAME, &shmobj_st);
    
    CHUNK_SIZE = shmobj_st.st_size / NUM_CH_SERVERS;    
    offset = shmobj_st.st_size / CHUNK_SIZE;

    open_file(FILENAME, CHUNK_SIZE);

//LECTURA

    int filed;
    char *pointer;

    filed = shm_open("2", O_RDONLY, 0); //Creamos un objeto de memoria compartido
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
    close(filed);

    return 0;
}
