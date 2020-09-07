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

int main(int argc, char *argv[]) {
  int cant_cnf_asig, i, c;
  int cant_cnf_unsol = argc - 1;

  if (argc <= 1) {
    printf("Error, debe pasar unicamente el path de la carpeta contenedora de los archivos\n");
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

  const char *sem_nameA = "/sem-entry";  //Para entrar a la zona critica
  const char *sem_nameB = "/sem-read";   //Para que vista sepa si hay para leer
  // nombre, crear y que no exista,permisos de RWX,valor iniical del sem
  sem_t *sem_entry = sem_open(sem_nameA, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
  if (sem_entry == SEM_FAILED) {
    printf("Failed to create the semaphore empty. Exiting...\n");
    exit(1);
  }
  sem_t *sem_read = sem_open(sem_nameB, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
  if (sem_read == SEM_FAILED) {
    printf("Failed to create the semaphore full. Exiting...\n");
    exit(1);
  }

  printf("%s %s %s", shm_name, sem_nameA, sem_nameB);
  fflush(stdout);
  sleep(5);  //espero al vista

  /*Variables for pipes*/
  int pipeMW[CANT_PROCESS][2];  //pipeMW - Master Writes
  int pipeMR[CANT_PROCESS][2];  //pipeMR - Master Reads
  pid_t cpid[CANT_PROCESS] = {0};
  int solved_queries[CANT_PROCESS] = {0};

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
      char *args[] = {"./slave", NULL};
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
  char aux[100] = {};
  for (i = 0, cant_cnf_asig = 0; i < CANT_PROCESS && cant_cnf_asig < argc - 1; i++)
    for (c = 0; c < INITIAL_FILES && cant_cnf_asig < argc - 1; c++, cant_cnf_asig++) {
      if (strlen(argv[cant_cnf_asig + 1]) >= 100) {
        perror("Filename too long");
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
          perror("vista: read");
          exit(EXIT_FAILURE);
        }
        sem_wait(sem_entry);
        strncpy((shm_ptr + current++)->arr, line, cant_read);
        if (write(output_fd, line, strlen(line)) == -1) {
          printf("Error write\n");
          exit(EXIT_FAILURE);
        }
        sem_post(sem_read);
        sem_post(sem_entry);
        cant_cnf_unsol--;
        solved_queries[k]++;
        if (solved_queries[k] >= INITIAL_FILES && cant_cnf_asig < argc - 1) {
          char aux[100] = {};
          strcpy(aux, argv[cant_cnf_asig + 1]);
          strcat(aux, "\n");
          if (write(pipeMW[k][WRITE], aux, strlen(aux)) == -1) {
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
  close(output_fd);
  sem_wait(sem_entry);
  strncpy((shm_ptr + current++)->arr, "*", 1);
  sem_post(sem_read);
  sem_post(sem_entry);
  sleep(1);
  int status;
  for (i = 0; i < CANT_PROCESS; i++) {
    close(pipeMW[i][WRITE]);
    close(pipeMR[i][READ]);
  }
  for (i = 0; i < CANT_PROCESS; i++)
    waitpid(cpid[i], &status, 0);

  if (sem_close(sem_entry) < 0) {
    printf("Error closing semaphore. Exiting...\n");
    exit(EXIT_FAILURE);
  }

  if (sem_close(sem_read) < 0) {
    printf("Error closing semaphore. Exiting...\n");
    exit(EXIT_FAILURE);
  }
  sem_unlink(sem_nameA);
  sem_unlink(sem_nameB);
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
  // remove the shared memory segment from the file system
  if (shm_unlink(shm_name) == -1) {
    perror("shm_unlink: No se pudo cerrar la shared memory");
    exit(EXIT_FAILURE);
  }
  return 0;
}