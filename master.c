#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#define CANT_PROCESS 5

#define READ 0
#define WRITE 1

int main(int argc, char* argv[]){
    if(argc <= 1){
        printf("Error, debe pasar unicamente el path de la carpeta contenedora de los archivos\n");
        exit(1);
    }
    int pipeMW[CANT_PROCESS][2],i; //pipeMW - Master Writes
    int pipeMR[CANT_PROCESS][2]; //pipeMR - Master Reads
    pid_t cpid[CANT_PROCESS] = {100};
    for (i=0; i<CANT_PROCESS ;i++){
        pipe(pipeMW[i]);
        pipe(pipeMR[i]);
        cpid[i]=fork();
        if(cpid[i] == -1){
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if(cpid[i]==0){
            close(pipeMW[i][WRITE]);
            close(pipeMR[i][READ]);
            dup2(pipeMW[i][READ], STDIN_FILENO); //El esclavo le llega por estandar input lo que escriben en el FD
            //dup2(pipeMR[i][WRITE], STDOUT_FILENO); //Salida estandar del esclavo se redirecciona al FD
            char *args[] = {"./slave",NULL};
            execvp(args[0], args);
            exit(1);//no deberia retornar              
        }
        else{
            close(pipeMW[i][READ]);
        }
    }

        for (i=0; i<CANT_PROCESS && i<argc-1 ;i++){
            write(pipeMW[i][WRITE], argv[i+1], strlen(argv[i+1]));
        }

        /*char *line = NULL;
        size_t linecap = 0;
        ssize_t linelen;

        for(i=0; i<CANT_PROCESS; i++){
            FILE* fp = fdopen(pipeMR[i][READ], "r");
            linelen = getline(&line, &linecap, fp);
            fwrite(line, linelen, 1, stdout);
        }*/
        

        int status;
        for (i=0; i<CANT_PROCESS;i++){
            close(pipeMW[i][WRITE]);
            close(pipeMR[i][READ]);
            //waitpid(cpid[i], &status, 0);
        }
        return 0;
}