// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <fcntl.h> /* For O_* constants */
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <unistd.h>

#define MAX_FILES 100
#define NAME_MAX 255

typedef struct buffer {
  char arr[256 * 6];
} buffer;

int main(int argc, char *argv[]) {
  char shm_name[NAME_MAX] = {}, sem_name[NAME_MAX] = {};
  char line[255];

  if (argc > 1) {
    int i;
    for (i = 1; i < 3; i++) {
      if (strlen(argv[i]) > NAME_MAX) {
        perror("vista: los directorios de SHM o SEM no pueden tener mas de 255 carac.");
        exit(EXIT_FAILURE);
      }
    }
    sprintf(shm_name, "/%s", argv[1]);
    sprintf(sem_name, "/%s", argv[2]);
  } else {
    if (read(STDIN_FILENO, line, 255) < 0) {
      perror("vista: Error en los nombres de SHM y SEM");
      exit(EXIT_FAILURE);
    }

    char tmp[2][15];
    char *token;
    int i;
    for (token = strtok(line, " "), i = 0; token != NULL; token = strtok(NULL, " "), i++)
      sprintf(tmp[i], "%s", token);

    sprintf(shm_name, "%s", tmp[0]);
    sprintf(sem_name, "%s", tmp[1]);
  }

  /* open the shared memory segment as if it was a file */
  int shm_fd = shm_open(shm_name, O_RDONLY, 0666);
  if (shm_fd == -1) {
    perror("vista: shm_open: No se pudo abrir la shared memory");
    exit(EXIT_FAILURE);
  }

  /* map the shared memory segment to the address space of the process */
  buffer *shm_ptr = mmap(0, sizeof(buffer) * MAX_FILES, PROT_READ, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap: No se pudo mapear la shared memory");
    close(shm_fd);
    exit(EXIT_FAILURE);
  }

  sem_t *sem_read = sem_open(sem_name, O_CREAT);
  if (sem_read == SEM_FAILED) {
    printf("vista:Failed to create the semaphore full. Exiting...\n");
    exit(EXIT_FAILURE);
  }

  int current = 0;  //Para saber en que posicion del buffer estoy
  char finish_vista = 1;
  char *aux;

  while (finish_vista) {
    if (sem_wait(sem_read) < 0) {
      perror("vista:sem wait");
      exit(EXIT_FAILURE);
    }

    aux = (shm_ptr + current++)->arr;
    if (aux[0] == '*')
      finish_vista = 0;
    else
      printf("%s\n", aux);
  }

  if (sem_close(sem_read) < 0) {
    printf("Error closing semaphore. Exiting...\n");
    exit(EXIT_FAILURE);
  }

  /* remove the mapped shared memory segment from the address space of the process */
  if (munmap(shm_ptr, sizeof(buffer) * MAX_FILES) == -1) {
    perror("munmap: No se pudo desmapear la shared memory");
    close(shm_fd);
    exit(EXIT_FAILURE);
  }

  /* close the shared memory segment as if it was a file */
  if (close(shm_fd) == -1) {
    perror("close: No se pudo cerrar el FD de la SM");
    exit(EXIT_FAILURE);
  }

  return 0;
}