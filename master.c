#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/select.h>

#define CANT_PROCESS 5

#define READ 0
#define WRITE 1

int main(int argc, char* argv[]){

    int cant_cnf_asig = argc;
    int cant_cnf_unsol = argc;
    if(argc <= 1){
        printf("Error, debe pasar unicamente el path de la carpeta contenedora de los archivos\n");
        exit(1);
    }

    /*Variables for pipes*/
    int pipeMW[CANT_PROCESS][2],i; //pipeMW - Master Writes
    int pipeMR[CANT_PROCESS][2]; //pipeMR - Master Reads
    pid_t cpid[CANT_PROCESS] = {100};
    char solved_queries[CANT_PROCESS];

    /*Variables for select*/
    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 500000;
    int max_fd = 0;


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
            if(pipeMR[i][READ] > max_fd)
                max_fd = pipeMW[i][READ];
            FD_SET(pipeMR[i][READ], &readfds);
        }
    }



        //Asignacion de 5 queries a cada esclavo
        int c;
        for (i=0; i<CANT_PROCESS && cant_cnf_asig>1 ;i++){
            for(c=0; c<5 && cant_cnf_asig>1 ; c++){
                printf("%s\n", argv[argc-cant_cnf_asig+1]);
                write(pipeMW[i][WRITE], argv[argc-cant_cnf_asig+1], strlen(argv[argc-cant_cnf_asig+1]));
                cant_cnf_asig--;
            }
        }
                
        int k, l;
        char line[100];
        size_t linecap = 0;
        ssize_t linelen;

        while(cant_cnf_unsol > 1){
            printf("sigo aca pa\n");
            select(max_fd+1, &readfds, NULL, NULL, &tv);
            for(k=0; k<CANT_PROCESS; k++){
                if(FD_ISSET(pipeMR[k][READ], &readfds)){
                    printf("HABIA ALGO PARA LEER");
                    for(l=0; l<6; l++){
                        linelen = read(pipeMR[k][READ], &line, 100);
                        printf("%s", line);
                        //Donde mandamos lo leido?
                        cant_cnf_unsol--;
                    }
                    solved_queries[k]++;
                    if(solved_queries[k]>=5){
                    write(pipeMW[k][WRITE], argv[argc], strlen(argv[argc])); 
                    cant_cnf_asig--;
                    }

                    FD_SET(pipeMR[k][READ],&readfds);
                }
            }
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