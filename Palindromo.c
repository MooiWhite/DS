#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

char buffer[] = ""; 
char an_buffer[] = "";

int isPalindrome(int x, char *myString,char *buffer, char *an_buffer){
    for (int i = 0; i<strlen(myString); i++){
        if (myString[i] != ' '){
            buffer[i] = myString[i];
            printf("%d,%c\n",i,myString[i]);
        }
        printf("BUFFER: %s\n", buffer);
    }
    
    for (int i = strlen(buffer); i>=0; i--){
        //printf("Len: %d", strlen(buffer));
        an_buffer[x] = buffer[i];
        x += 1;
        //printf("%d buffer: %c\n",x,an_buffer[x]);
    }    
        printf("buffer: %s\n",buffer);
    for (int i = 0; i<strlen(myString); i++){
        if (buffer[i]!=an_buffer[i]){
            printf("Esto no es un palindromo!\n");
            exit(1);
        }
    }
}

int main(void){
    char string_1[] = "le avisara sara si va el";
    isPalindrome(0,string_1, buffer, an_buffer);
return 0;
}
