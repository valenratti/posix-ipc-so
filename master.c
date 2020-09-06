#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CANT_PROCESS 3
#define INITIAL_FILES 3

#define READ 0
#define WRITE 1

int main(int argc, char* argv[]) {
  int cant_cnf_asig, i, c;
  int cant_cnf_unsol = argc - 1;
  if (argc <= 1) {
    printf("Error, debe pasar unicamente el path de la carpeta contenedora de los archivos\n");
    exit(1);
  }

  /*Variables for pipes*/
  int pipeMW[CANT_PROCESS][2];  //pipeMW - Master Writes
  int pipeMR[CANT_PROCESS][2];  //pipeMR - Master Reads
  pid_t cpid[CANT_PROCESS] = {100};
  int solved_queries[CANT_PROCESS];

  /*Variables for select*/
  fd_set readfds, writefds;
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  int max_fd = 0;
  //fflush(stdout);

  for (i = 0; i < CANT_PROCESS; i++) {
    pipe(pipeMW[i]);
    pipe(pipeMR[i]);
    cpid[i] = fork();
    if (cpid[i] == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
    }
    if (cpid[i] == 0) {
      close(pipeMW[i][WRITE]);
      close(pipeMR[i][READ]);
      dup2(pipeMW[i][READ], STDIN_FILENO);    //El esclavo le llega por estandar input lo que escriben en el FD
      dup2(pipeMR[i][WRITE], STDOUT_FILENO);  //Salida estandar del esclavo se redirecciona al FD
      char* args[] = {"./slave", NULL};
      execvp(args[0], args);
      perror("exec");
      exit(EXIT_FAILURE);
    } else {
      close(pipeMW[i][READ]);
      close(pipeMR[i][WRITE]);
      if (pipeMR[i][READ] > max_fd)
        max_fd = pipeMR[i][READ];
      FD_SET(pipeMR[i][READ], &readfds);
    }
  }

  //Asignacion de INITIAL_FILES queries a cada esclavo

  for (i = 0, cant_cnf_asig = 0; i < CANT_PROCESS && cant_cnf_asig < argc - 1; i++)
    for (c = 0; c < INITIAL_FILES && cant_cnf_asig < argc - 1; c++, cant_cnf_asig++) {
      char aux[100] = {};
      strcpy(aux, argv[cant_cnf_asig + 1]);
      strcat(aux, "\n");
      write(pipeMW[i][WRITE], aux, strlen(aux));
    }

  int k;
  char line[100];
  size_t linecap = 0;
  ssize_t linelen;

  while (cant_cnf_unsol > 0) {
    select(max_fd + 1, &readfds, NULL, NULL, NULL);

    for (k = 0; k < CANT_PROCESS && cant_cnf_unsol > 0; k++) {  //unsolve = argc - 1 - asignated
      if (FD_ISSET(pipeMR[k][READ], &readfds)) {
        linelen = read(pipeMR[k][READ], &line, 256);
        printf("%s\n", line);
        //Donde mandamos lo leido?
        cant_cnf_unsol--;
        solved_queries[k]++;
        if (solved_queries[k] >= INITIAL_FILES && cant_cnf_asig < argc - 1) {
          char aux[100] = {};
          strcpy(aux, argv[cant_cnf_asig + 1]);
          strcat(aux, "\n");
          printf("%s este\n", aux);
          if (write(pipeMW[i][WRITE], aux, strlen(aux)) == -1) {
            printf("Error write\n");
            exit(EXIT_FAILURE);
          }
          cant_cnf_asig++;
        }
      }
    }
    for (i = 0; i < CANT_PROCESS; i++)
      FD_SET(pipeMR[i][READ], &readfds);
  }

  int status;
  for (i = 0; i < CANT_PROCESS; i++) {
    close(pipeMW[i][WRITE]);
    close(pipeMR[i][READ]);
    //waitpid(cpid[i], &status, 0);
  }
  return 0;
}