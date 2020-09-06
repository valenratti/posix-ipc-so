#include <fcntl.h> /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <semaphore.h>

#define MAX_FILES 100

typedef struct buffer
{
  char arr[256 * 6];
} buffer;

int main(int argc, char *argv[]) {
  char shm_name[15], sem_entry_name[15],sem_read_name[15];

  if (argc > 1) {
    sprintf(shm_name, "/%s", argv[1]);
    sprintf(shm_name, "/%s", argv[2]);
    sprintf(shm_name, "/%s", argv[3]);
  }
  else
  {
    scanf(shm_name, "/%s");
    scanf(sem_entry_name, "/%s");
    scanf(sem_read_name,"/%s");
  }

  /* open the shared memory segment as if it was a file */
  int shm_fd = shm_open(shm_name, O_RDONLY, 0666);
  if (shm_fd == -1) {
    perror("shm_open: No se pudo abrir la shared memory");
    exit(EXIT_FAILURE);
  }

  /* map the shared memory segment to the address space of the process */
  buffer *shm_ptr = mmap(0, sizeof(buffer) * MAX_FILES, PROT_READ, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED)
  {
    perror("mmap: No se pudo mapear la shared memory");
    close(shm_fd);
    shm_unlink(shm_name);
    exit(EXIT_FAILURE);
  }

  /*
  *  ToDo
  */

  // nombre, crear y que no exista,permisos de RWX,valor iniical del sem
  sem_t *sem_entry = sem_open(sem_entry_name, O_CREAT);
  if (sem_entry == SEM_FAILED)
  {
    printf("Failed to create the semaphore empty. Exiting...\n");
    exit(EXIT_FAILURE);
  }
  sem_t *sem_read = sem_open(sem_read_name, O_CREAT);
  if (sem_read == SEM_FAILED)
  {
    printf("Failed to create the semaphore full. Exiting...\n");
    exit(EXIT_FAILURE);
  }

  int current = 0; //Para saber en que posicion del buffer estoy
  char finish_vista = 1;
  char *aux;

  while (finish_vista) {
    sem_wait(sem_read);
    sem_wait(sem_entry);
    aux = (shm_ptr + current++)->arr;
    if (aux[0] == EOF)
      finish_vista = 0;
    else 
      printf("%s\n", (shm_ptr + current++)->arr);
    sem_post(sem_entry);
  }

  if (sem_close(sem_entry < 0)){
    printf("Error closing semaphore. Exiting...\n");
    exit(EXIT_FAILURE);
  }

  if (sem_close(sem_read < 0)){
    printf("Error closing semaphore. Exiting...\n");
    exit(EXIT_FAILURE);
  }

  /* remove the mapped shared memory segment from the address space of the process */
  if (munmap(shm_ptr, sizeof(buffer) * MAX_FILES) == -1)
  {
    perror("munmap: No se pudo desmapear la shared memory");
    close(shm_fd);
    exit(EXIT_FAILURE);
  }

  /* close the shared memory segment as if it was a file */
  if (close(shm_fd) == -1)
  {
    perror("close: No se pudo cerrar el FD de la SM");
    exit(EXIT_FAILURE);
  }

  return 0;

}