#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static int count = 10;
//Una variable cuyo valor puede ser modificado por diferentes funciones
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *thread1_routine(void *unused){
    for(int i=0; i<100; i++){
        
        pthread_mutex_lock(&mutex);
        count++;
        printf("%d\n",count);
        pthread_mutex_unlock(&mutex);
    }
    //pthread_exit(NULL);
}

void *thread2_routine(void *unused){
    for(int i=0; i<200; i++){
        
        pthread_mutex_lock(&mutex);
        printf("%d\n",count);
        count = count%3;
        pthread_mutex_unlock(&mutex);
    }
    //pthread_exit(NULL);
}

void *thread3_routine(void *unused){
    for(int i=0; i<5; i++){
        
        pthread_mutex_lock(&mutex);
        
        count = count*2;
        printf("%d\n",count);
        pthread_mutex_unlock(&mutex);
    }
    //pthread_exit(NULL);
}


void errorExit(char *strerr){
    perror(strerr);
    exit(1);
}

int main(void){
    pthread_t thread1, thread2, thread3;
    
    if (0 != pthread_create(&thread1,NULL, thread1_routine,NULL))
        errorExit("thread1 connot be created");

    if (0 != pthread_create(&thread2,NULL, thread2_routine,NULL))
        errorExit("thread2 connot be created");

    if (0 != pthread_create(&thread3,NULL, thread3_routine,NULL))
            errorExit("thread2 connot be created");

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    

    printf("Count value %d \n", count);
    return 0;
}
