/*-------------------Universidad Nacional Autónoma de México-------------------------
---------------------------Facultad de Ingeniería------------------------------------
........................Sistemas Distribuidos 2022-2.................................
                             Autor: Equipo 08
--------------------------The Google File System-------------------------------------
*/

//Forma de compilación: gcc GFS.c -lrt -pthread -luuid -o GFS

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

#define FILENAME "test.txt"
#define SMOBJ_SIZE 400
#define LEVEL_REP 3
#define NUM_CH_SERVERS 4
#define NUM_CLIENTS 2

static size_t CHUNK_SIZE;
static size_t offset;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct Master {
    char* filename;
    unsigned char IDChunk;
    size_t* IDChunkServer;
    size_t start_byte;
    size_t end_byte;
}Master;

//Insert a row on master table
int insert_on_master(char* name, unsigned char IDChunk, size_t* IDserver, Master* master, size_t start, size_t end) {
    master->filename = name;
    master->IDChunk = IDChunk;
    master->IDChunkServer = IDserver;
    master->start_byte = start;
    master->end_byte = end;
    return 0;
}

//Initialize only a chunkserver
int init_chunkserver(char* ID) {
    int fd;
    fd = shm_open(ID, O_CREAT | O_RDWR, 00600); //Creamos un objeto de memoria compartido
    if (fd == -1) {
        printf("error, shared memory could not be created \n");
        exit(1);
    }
    if (1 == ftruncate(fd, SMOBJ_SIZE)) {
        printf("error, shared could not be sized\n");
        exit(1);
    }
    return fd;
}

//This function is not used, but it could be useful to represent append function of GFS
void append_on_chsvr(int fd, char* buf) {
    printf("Into chunk I received this: %s , Size: %u\n", buf, strlen(buf));
    char* ptr;
    ptr = mmap(0, strlen(buf), PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        printf("Error in memory mapping\n");
        exit(1);
    }
    memcpy(ptr, buf, strlen(buf));
}

/*
  It Replies a bufer of information on certain number of chunkservers
  number of chunkservers with information replied = LEVEL OF REPLICATION
*/
void replication(char* buf) {
    int fd;
    char* ptr;
    char key[2];

    for (int i = 1; i <= LEVEL_REP; i++) {
        sprintf(key, "%d", i); //Convert int to string and put the result in ID
        fd = shm_open(key, O_RDWR, 0); //Leemos un objeto de memoria compartido
        printf("%s --> chunk server: %d\n", buf, i);
        ptr = mmap(0, strlen(buf), PROT_WRITE, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) {
            printf("Error in memory mapping\n");
            exit(1);
        }
        strncat(ptr, buf, strlen(buf));
        close(fd);
    }
}

/*
  It saves the chunkservers number as an array of every
  chunkserver

*/
size_t* save_server_index(size_t level_rep) {
    size_t* array_index = malloc(level_rep * sizeof(level_rep));
    for (size_t i = 0; i < level_rep; i++) {
        array_index[i] = i + 1;
    }
    return array_index;
}

/*
  Create, initialize chunkserves
  It copies information of a file on chunkservers
  It replies the information in order to the number of LEVEL REPLICATION
*/
int open_file(char* filename, size_t chunk_size, Master* master, size_t offset) {
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

    //Conversion file to chunks on chunkservers

    //Initialize chunkservers
    for (int i = 1; i <= NUM_CH_SERVERS; i++) {
        fgets(buf, chunk_size + 1, ptr);//Read a chunk of a file
        sprintf(ID, "%d", i); //Convert int to string and put the result in ID
        fd = init_chunkserver(ID); //Init a chunkserver with its ID
        //write_chunkserver(fd,buf); //Write on an specific chunkserver with fd
    }

    ptr = fopen(filename, "r");
    size_t count = 0;
    printf("\nSTARTING REPLICATION: \n");
    //Replication on chunkservers
    for (int i = 1; i <= offset; i++) {
        fgets(buf, chunk_size + 1, ptr);//Read a chunk of a file
        replication(buf);//Replication

        for (int j = 1; j <= LEVEL_REP; j++) {
            uuid_generate_random(binuuid);
            insert_on_master(FILENAME, *binuuid, save_server_index(LEVEL_REP), &master[i], chunk_size * count, (chunk_size * count) + chunk_size);
        }
        count++;
    }
}

/*
  It reads only a chunk of the first chunkserver.
  The reason is because the GFS reads on near chunkserver
  This parameter is an example to demonstrate this fact, but
  it could be changed.
  It reads an offset
*/
void read_chunk(/*void *id_chserver*/int id_chserver, size_t start, size_t end) {
    int filed;
    char* pointer;
    char* ID;
    struct stat shmobj_st;

    sprintf(ID, "%d", id_chserver);
    filed = shm_open(ID/*ID CHUNK_SERVER*/, O_RDONLY, 0); //READING SHARED OBJECT MEM

    if (filed == -1) {
        printf("Error file descriptor %s\n", strerror(errno));
        exit(1);
    }

    if (fstat(filed, &shmobj_st) == -1) {
        printf("Error fstat\n");
        exit(1);
    }

    pointer = mmap(NULL, 2, PROT_READ, MAP_SHARED, filed, 0);
    if (pointer == MAP_FAILED) {
        printf("Mapp failed in read mapping process\n");
        exit(1);
    }

    for (int i = start; i < end; i++) {
        printf("%c", pointer[i]);
    }

    printf("\n\n");
    close(filed);

}

/*
  It prints al the information of the row
  that receives as parameter, but also
  it calls read_chunk funtion to read
  an specific chunk
*/
void* read_row(void* master) {
    Master* a_master = master;
    printf("\nTHIS IS:\n");
    printf("Row master test [FILENAME]: %s\n", a_master->filename);
    printf("Row master test [IDChunk]: %d\n", a_master->IDChunk);
    printf("Row master test [START_BYTE] %d\n", a_master->start_byte);
    printf("Row master test [END_BYTE] %d\n", a_master->end_byte);
    printf("\n\n");
    read_chunk(1, a_master->start_byte, a_master->end_byte);
    return NULL;
}

/*
  We used mutex to avoid segment violations on
  Shared Objects Memory that were created.
  This function receives a master's row like
  parameter, and read a row
*/
void* client_routine(void* unused) {
    pthread_mutex_lock(&mutex);
    read_row(unused);
    pthread_mutex_unlock(&mutex);

}

//ERROR FUNCTION
/*
  If thread reading cannot be created
*/
void errorExit(char* strerr) {
    perror(strerr);
    exit(1);
}

//PRINTING
/*
   This function prints the rows of the master table
*/
void print_MasterTable(Master* master, size_t offset) {
    printf("\nPRINTING TABLE MASTER\n");
    for (int i = 1; i <= offset; i++) {
        printf("Row master test [FILENAME]: %s\n", master[i].filename);
        printf("Row master test [IDChunk]: %d\n", master[i].IDChunk);
        printf("Row master test [INDEXES CHUNK]:");
        for (size_t j = 0; j < LEVEL_REP; j++) {
            printf("%d,", master[i].IDChunkServer[j]);
        }
        printf("\n");
        printf("Row master test [START_BYTE] %d\n", master[i].start_byte);
        printf("Row master test [END_BYTE] %d\n", master[i].end_byte);
        printf("---------------------------------------\n");
    }
}

int main(void) {
    //char ID[10];
    //int fd;
    int client_chunk_id; //IDchunk that we can choose

    struct stat shmobj_st; //for get the offset
    stat(FILENAME, &shmobj_st);

    //Create a master table with 10 as number of chunks
    Master master[10];

    //Calculate the chunk_size = Number of bytes that every ch_server will have
    CHUNK_SIZE = shmobj_st.st_size / NUM_CH_SERVERS;
    //NUmber of of chunks that totally we will have
    offset = shmobj_st.st_size / CHUNK_SIZE;

    //It opens a file to read it and copy its information in Shared Ob Mem
    open_file(FILENAME, CHUNK_SIZE, master, offset);

    //Print our Master table that have all the metadata of chunkservers and chunks
    print_MasterTable(master, offset);

    //We are clients, so we can choose an ID that we want to read
    printf("\nEscoje un ID_chunk de la tabla: ");
    scanf("%d", &client_chunk_id);

    //We are creating a certain number of POSIX threads with NUM_CLIENTS
    pthread_t clients[NUM_CLIENTS];

    //Creation of threads (clients), its execution and joining
    /*
    Every thread only executes a routine called "client_routine"
    its fucntion is to read an specific chunk with only
    the ID_CHUNK that could be seen in the Master table
    shown before.
    */
    for (int i = 1; i <= offset; i++) {
        if (master[i].IDChunk == client_chunk_id) {
            for (int j = 1; j <= NUM_CLIENTS; j++) {
                if (0 != pthread_create(&clients[j], NULL, client_routine, (void*)&master[i]))
                    errorExit("thread reading cannot be created");
            }
            for (int i = 1; i <= NUM_CLIENTS; i++) {
                pthread_join(clients[i], NULL);
            }

        }
    }

    return 0;
}
