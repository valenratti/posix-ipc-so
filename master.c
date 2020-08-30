#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#define CANT_PROCESS 5

#define READ 0
#define WRITE 1


int main(int argc, char* argv[]){
    if(argc != 2){
        printf("Error, debe pasar unicamente el path de la carpeta contenedora de los archivos\n");
        exit(1);
    }
    DIR *current = opendir(argv[1]);
    if (current == NULL)
    {
        printf("Can't find the directory!\n");
        exit(1);
    }else{
        struct dirent *entry;
        /*while ((entry = readdir(current)) != NULL)
        {
            if (entry->d_type == DT_DIR)
            {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..")!=0)
                {
                    printf("d");
                    printTabs(level);
                    printf("%s\n", entry->d_name);
                    char path[1024];
                    snprintf(path, sizeof path, "%s/%s", dir_name, entry->d_name);
                    listRec(path, level + 1);
                }
            }
            else
            {
                
            }
        }*/
        int mypipeSet[CANT_PROCESS][2],i;
        int mypipeRead[CANT_PROCESS][2];
        pid_t cpid[CANT_PROCESS] = {100};
        for (i=0; i<CANT_PROCESS ;i++){
            pipe(mypipeSet[i]);
            pipe(mypipeRead[i]);
            cpid[i]=fork();
            if(cpid[i] == -1){
                perror("fork");
                exit(EXIT_FAILURE);
            }
            if(cpid[i]==0){
                close(mypipeSet[i][WRITE]);
                close(mypipeRead[i][READ]);
                dup2(mypipeSet[i][READ], STDIN_FILENO);
                dup2(mypipeRead[i][WRITE], STDOUT_FILENO);

                char *args[] = {"./slave",NULL};
                execvp(args[0], args);
                exit(1);//no deberia retornar              
            }
            else{
                close(mypipeSet[i][READ]);
            }
        }
        

        

        char *string = "Hola\n";

        for (i=0; i<CANT_PROCESS ;i++){
            write(mypipeSet[i][WRITE], string, 5);
        }

        /*char *line = NULL;
        size_t linecap = 0;
        ssize_t linelen;

        for(i=0; i<CANT_PROCESS; i++){
            FILE* fp = fdopen(mypipeRead[i][READ], "r");
            linelen = getline(&line, &linecap, fp);
            fwrite(line, linelen, 1, stdout);
        }*/
        

        int status;
        for (i=0; i<CANT_PROCESS;i++){
            close(mypipeSet[i][WRITE]);
            close(mypipeRead[i][READ]);
            //waitpid(cpid[i], &status, 0);
        }
    }
    
        return 0;
}