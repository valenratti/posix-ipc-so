// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <fcntl.h> /* For O_* constants */
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CANT_PROCESS 3
#define INITIAL_FILES 3
#define MAX_FILES 100

#define READ 0
#define WRITE 1

typedef struct buffer {
  char arr[256 * 6];
} buffer;

void failEndVista(buffer *shm_ptr, int current, sem_t *sem_read, const char *shm_name, const char *sem);

int main(int argc, char *argv[]) {
  int cant_cnf_asig, i, c;
  int cant_cnf_unsol = argc - 1;

  if (argc <= 1) {
    printf("Error, debe pasar unicamente el path de la carpeta contenedora de los archivos\n");
    exit(1);
  }
  if (argc - 1 > MAX_FILES) {
    printf("Error, se pueden analizar hasta 100 archivos\n");
    exit(1);
  }
  char outputfile[] = "output.txt";
  int output_fd = open(outputfile, O_CREAT | O_RDWR, S_IRWXU);
  if (output_fd == -1) {
    perror("master: No se pudo abrir el archivo");
    exit(EXIT_FAILURE);
  }

  const char *shm_name = "/buffer";  // file name

  /* create the shared memory segment as if it was a file */
  int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) {
    perror("master: shm_open: No se pudo abrir la shared memory");
    exit(EXIT_FAILURE);
  }

  /* map the shared memory segment to the address space of the process */
  buffer *shm_ptr = mmap(0, sizeof(buffer) * MAX_FILES, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap: No se pudo mapear la shared memory");
    close(shm_fd);
    shm_unlink(shm_name);
    exit(EXIT_FAILURE);
  }

  ftruncate(shm_fd, sizeof(buffer) * MAX_FILES);

  const char *sem_name = "/sem";  //Para que vista sepa si hay para leer

  // nombre, crear y que no exista, permisos de RWX, valor iniical del sem
  sem_t *sem_read = sem_open(sem_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
  if (sem_read == SEM_FAILED) {
    printf("master: Failed to create the semaphore full. Exiting...\n");
    exit(1);
  }

  printf("%s %s ", shm_name, sem_name);
  fflush(stdout);
  sleep(2);  //waiting for vista

  /*Variables for pipes*/
  int pipeMW[CANT_PROCESS][2];  //pipeMW - Master Writes
  int pipeMR[CANT_PROCESS][2];  //pipeMR - Master Reads
  pid_t cpid[CANT_PROCESS] = {0};
  int solved_queries[CANT_PROCESS] = {0};

  /*Variables for select*/
  fd_set readfds;
  FD_ZERO(&readfds);
  int max_fd = 0;

  for (i = 0; i < CANT_PROCESS; i++) {
    if (pipe(pipeMW[i]) < 0 || pipe(pipeMR[i]) < 0) {
      failEndVista(shm_ptr, 0, sem_read, shm_name, sem_name);
      perror("pipe");
      exit(EXIT_FAILURE);
    }

    cpid[i] = fork();
    if (cpid[i] == -1) {
      perror("fork");
      failEndVista(shm_ptr, 0, sem_read, shm_name, sem_name);
      exit(EXIT_FAILURE);
    }

    if (cpid[i] == 0) {
      if (close(pipeMW[i][WRITE]) < 0 || close(pipeMR[i][READ]) < 0) {
        perror("close pipe");
        exit(EXIT_FAILURE);
      }

      if (dup2(pipeMW[i][READ], STDIN_FILENO) < 0 || dup2(pipeMR[i][WRITE], STDOUT_FILENO) < 0) {
        perror("dup");
        exit(EXIT_FAILURE);
      }

      char *args[] = {"./slave", NULL};
      execvp(args[0], args);
      perror("exec");
      exit(EXIT_FAILURE);
    } else {
      if (close(pipeMW[i][READ]) < 0 || close(pipeMR[i][WRITE]) < 0) {
        perror("close pipe");
        failEndVista(shm_ptr, 0, sem_read, shm_name, sem_name);
        exit(EXIT_FAILURE);
      }
      if (pipeMR[i][READ] > max_fd)
        max_fd = pipeMR[i][READ];
      FD_SET(pipeMR[i][READ], &readfds);
    }
  }

  /*Asignation of INITIAL_FILES queries to each slave*/
  char aux[100] = {};
  for (i = 0, cant_cnf_asig = 0; i < CANT_PROCESS && cant_cnf_asig < argc - 1; i++)
    for (c = 0; c < INITIAL_FILES && cant_cnf_asig < argc - 1; c++, cant_cnf_asig++) {
      if (strlen(argv[cant_cnf_asig + 1]) >= 100) {
        perror("Filename too long");
        failEndVista(shm_ptr, 0, sem_read, shm_name, sem_name);
        exit(EXIT_FAILURE);
      }
      strcpy(aux, argv[cant_cnf_asig + 1]);
      strcat(aux, "\n");
      write(pipeMW[i][WRITE], aux, strlen(aux));
    }

  int k;
  char line[255];
  int current = 0;
  ssize_t cant_read;
  while (cant_cnf_unsol > 0) {
    select(max_fd + 1, &readfds, NULL, NULL, NULL);

    for (k = 0; k < CANT_PROCESS && cant_cnf_unsol > 0; k++) {  //unsolve = argc - 1 - asignated
      if (FD_ISSET(pipeMR[k][READ], &readfds)) {
        cant_read = read(pipeMR[k][READ], &line, 255);

        if (cant_read == -1) {
          perror("master: read");
          failEndVista(shm_ptr, current, sem_read, shm_name, sem_name);
          exit(EXIT_FAILURE);
        }

        strncpy((shm_ptr + current++)->arr, line, cant_read);

        if (write(output_fd, line, cant_read) == -1) {
          perror("master:Error write");
          failEndVista(shm_ptr, current, sem_read, shm_name, sem_name);
          exit(EXIT_FAILURE);
        }

        if (sem_post(sem_read) < 0) {
          perror("master:sem post");
          failEndVista(shm_ptr, current, sem_read, shm_name, sem_name);
          exit(EXIT_FAILURE);
        }

        cant_cnf_unsol--;
        solved_queries[k]++;

        if (solved_queries[k] >= INITIAL_FILES && cant_cnf_asig < argc - 1) {
          char aux[100] = {};

          if (strlen(argv[cant_cnf_asig + 1]) >= 100) {
            perror("Filename too long");
            failEndVista(shm_ptr, current, sem_read, shm_name, sem_name);
            exit(EXIT_FAILURE);
          }

          strcpy(aux, argv[cant_cnf_asig + 1]);
          strcat(aux, "\n");

          if (write(pipeMW[k][WRITE], aux, strlen(aux)) == -1) {
            perror("Error write");
            failEndVista(shm_ptr, current, sem_read, shm_name, sem_name);
            exit(EXIT_FAILURE);
          }

          cant_cnf_asig++;
        }
      }
    }

    for (i = 0; i < CANT_PROCESS; i++)
      FD_SET(pipeMR[i][READ], &readfds);
  }

  if (close(output_fd) < 0) {
    perror("close");
    failEndVista(shm_ptr, current, sem_read, shm_name, sem_name);
    exit(EXIT_FAILURE);
  }
  strncpy((shm_ptr + current++)->arr, "*", 1);
  if (sem_post(sem_read) < 0) {
    perror("master:sem post");
    failEndVista(shm_ptr, current, sem_read, shm_name, sem_name);
    exit(EXIT_FAILURE);
  }
  sleep(1);

  for (i = 0; i < CANT_PROCESS; i++) {
    if (close(pipeMW[i][WRITE]) < 0 || close(pipeMR[i][READ]) < 0) {
      perror("pipe close");
      exit(EXIT_FAILURE);
    }
  }

  int status;
  for (i = 0; i < CANT_PROCESS; i++)
    waitpid(cpid[i], &status, 0);

  if (sem_close(sem_read) < 0) {
    perror("SEM close");
    exit(EXIT_FAILURE);
  }

  if (sem_unlink(sem_name) < 0) {
    perror("SEM Unlink");
    exit(EXIT_FAILURE);
  }

  /* remove the mapped shared memory segment from the address space of the process */
  if (munmap(shm_ptr, sizeof(buffer) * MAX_FILES) == -1) {
    perror("munmap: No se pudo desmapear la shared memory");
    close(shm_fd);
    shm_unlink(shm_name);
    exit(EXIT_FAILURE);
  }

  /* close the shared memory segment as if it was a file */
  if (close(shm_fd) == -1) {
    perror("close: No se pudo cerrar el FD de la SM");
    shm_unlink(shm_name);
    exit(EXIT_FAILURE);
  }

  /* remove the shared memory segment from the file system */
  if (shm_unlink(shm_name) == -1) {
    perror("shm_unlink: No se pudo cerrar la shared memory");
    exit(EXIT_FAILURE);
  }

  return 0;
}

/*Function to make vista return and close semaphores and shared memory in case of error*/
void failEndVista(buffer *shm_ptr, int current, sem_t *sem_read, const char *shm_name, const char *sem) {
  strncpy((shm_ptr + current)->arr, "*", 1);
  sem_post(sem_read);
  sem_unlink(sem);
  munmap(shm_ptr, sizeof(buffer) * MAX_FILES);
  shm_unlink(shm_name);
}
