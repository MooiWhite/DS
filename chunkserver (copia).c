#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <uuid/uuid.h>
#include <errno.h>

#define SMOBJ_NAME "/myMemoryObj"
#define FILENAME "test.txt"
#define SMOBJ_SIZE 400
#define LEVEL_REP 3
#define CHUNK_SIZE 10

//int const LEVEL_REP = 3;

typedef struct Master {
    char *filename;
    unsigned char IDChunk;
    int *IDChunkServer;
}Master;

int insert_on_master(char* name,unsigned char IDChunk,int *IDserver,Master* master){
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
    //char buf[]= "Hi this is a simple and normal text\n";
    printf("Into chunk I received this: %s , Size: %u\n",buf,strlen(buf));
    char *ptr;
    ptr =  mmap(0, strlen(buf), PROT_WRITE, MAP_SHARED,fd, 0);
    //printf("Size: %d\n",sizeof(buf));
    if (ptr == MAP_FAILED){
        printf("Error in memory mapping\n");
        exit(1);
    }
    memcpy(ptr,buf,strlen(buf));
}

//Está mal, que pasa cuando el ID es 3 sefuirá sumando hasta un ID 6
//NO se necesita adelante,se necesita sorteado
void replication(int ID, char* buf){
    int fd;
    char *ptr;
    char key[2];
    //char prueba[2] = "0";

    for (int i=0; i<=LEVEL_REP;i++){
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


int open_file(char* filename){
    char ID[10];
    char ID_CHUNK[2];
    int fd;
    char buf[CHUNK_SIZE];
    size_t offset;
    uuid_t binuuid;

    struct stat st;
    stat (filename, &st);

    FILE* ptr = fopen(filename, "r");
        if (ptr == NULL) {
            printf("no such file");
            return 0;
        }  
    
    offset = st.st_size / CHUNK_SIZE;
    printf("OFFFFFSETTTTT%d\n", offset);
    printf("CHUNKSIZE: %d\n",CHUNK_SIZE);
    printf("SIZE: %ld\n",st.st_size);
    
    //Conversion file to chunks on chunkservers

    //Initialize chunkservers
    for (int i=0;i<offset;i++){
        fgets(buf, CHUNK_SIZE, ptr);//Read a chunk of a file
        //printf("%s\n", buf);
        sprintf(ID,"%d",i); //Convert int to string and put the result in ID
        fd = init_chunkserver(ID); //Init a chunkserver with its ID
        write_chunkserver(fd,buf); //Write on an specific chunkserver with fd
        //FIRST REPLICATION
        //printf("FUe"); 
        //replication(fd,i,buf);
    }

    ptr = fopen(filename, "r");

    int arrayTest[] = {0,1};
    char *p;
    Master master[10];//We will have a certain number of rows as files we have
    char prueba[]= "Hi this is a simple and normal text\n";

    //Replication on chunkservers
    for (int i=0; i<4; i++){
        fgets(buf, CHUNK_SIZE, ptr);//Read a chunk of a file
        //printf("BUFFER:%s",buf);
        sprintf(ID,"%d",i); //Convert int to string and put the result in ID
        printf("NIVEL EXTERNO: %d\n",i);
        fd = shm_open(ID, O_RDWR, 0); //Leemos un objeto de memoria compartido
        //fd = shm_open(SMOBJ_NAME, O_RDWR, 0);
        if (fd == -1){
            printf("error, shared memory could not be readed \n");
            exit(1);
        }
        replication(i,buf);
        uuid_generate_random(binuuid);
        insert_on_master(FILENAME,*binuuid,arrayTest,&master[i]);
        printf("Row master test [FILENAME]: %s\n",master[i].filename);
        printf("Row master test [IDChunk]: %d\n",master[i].IDChunk);
        //printf("Row master test [IDChunkServer]:");
        close(fd);
    }

    /*char *p;
    Master master[10];//We will have a certain number of rows as files we have
    char prueba[]= "Hi this is a simple and normal text\n";

    insert_on_master(filename, ID CHUNK ,arrayTest,&master[0]);
    printf("Row master test [FILENAME]: %s\n",master[0].filename);
    printf("Row master test [IDChunk]: %d\n",master[0].IDChunk);
    printf("Row master test [IDChunkServer]:");
    for (int i = 0; i<2;i++){
        printf("%d,",master[0].IDChunkServer[i]);
    }*/
}


int main(void){
    char ID[10];
    int fd;
    struct stat shmobj_st;

    open_file(FILENAME);
/*FILE* ptr = fopen(FILENAME, "r");
    if (ptr == NULL) {
        printf("no such file");
        return 0;
    }    

    char buf[30];
    for (int i=0;i<4;i++){
        fgets(buf, 30, ptr);//Read a chunk of a file
        //printf("%s\n", buf);
        sprintf(ID,"%d",i); //Convert int to string and put the result in ID
        fd = init_chunkserver(ID); //Init a chunkserver with ID returned
        write_chunkserver(fd,buf); //Write on an specific chunkserver with fd
        //printf("FUe");
        //replication(fd,i,buf);
    }
    ptr = fopen(FILENAME, "r");

    for (int i=0; i<4; i++){
        fgets(buf, 30, ptr);//Read a chunk of a file
        //printf("BUFFER:%s",buf);
        sprintf(ID,"%d",i); //Convert int to string and put the result in ID
        printf("NIVEL EXTERNO: %d\n",i);
        fd = shm_open(ID, O_RDWR, 0); //Creamos un objeto de memoria compartido
        //fd = shm_open(SMOBJ_NAME, O_RDWR, 0);
        if (fd == -1){
            printf("error, shared memory could not be readed \n");
            exit(1);
        }
        replication(i,buf);
    }*/

    //Insert on master 
    /*char *p;
    Master master[10];//We will have a certain number of rows as files we have
    char prueba[]= "Hi this is a simple and normal text\n";

    //int arrayTest[] = {0,1};
    //uuid_generate_random(binuuid);
    //insert_on_master(FILENAME,*binuuid,arrayTest,&master[0]);
    //insert_on_master(FILENAME, 0,arrayTest,&master[0]);
    printf("Row master test [FILENAME]: %s\n",master[0].filename);
    printf("Row master test [IDChunk]: %d\n",master[0].IDChunk);
    printf("Row master test [IDChunkServer]:");
    for (int i = 0; i<2;i++){
        printf("%d,",master[0].IDChunkServer[i]);
    }*/

    
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
    /*for (int i = CHUNK_SIZE; i<2*CHUNK_SIZE; i++){
        printf("%c",pointer[i]);
    }*/
    printf("%s \n", pointer);
    close(filed);


    return 0;
}
