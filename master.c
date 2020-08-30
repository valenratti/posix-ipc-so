#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#define CANT_PROCESS 5

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
        int mypipe[CANT_PROCESS][2],i;
        pid_t cpid[CANT_PROCESS];
        for (i=0; i<CANT_PROCESS && cpid[i] != 0;i++){
            pipe(mypipe[i]);
            cpid[i]=fork();
            if(cpid[i] == -1){
                perror("fork");
                exit(EXIT_FAILURE);
            }
            if(cpid[i]==0){
                printf("Soy el proceso hijo. Mi PID es %d.\n", getpid());
                char *args[] = {"./slave",NULL};
                execvp(args[0], args);
                exit(1);//no deberia retornar              
            }
        }

        while((entry = readdir(current)) != NULL){
            
        }
    }
}