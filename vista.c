#include <fcntl.h> /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */

struct buf_msg {
  char buf[6][256];
};

int main(int argc, char *argv[]) {
  char shm_name[15];

  if (argc > 1) {
    sprintf(shm_name, "/%s", argv[1]);
  } else {
    scanf(shm_name, "/%s");
  }

  /* open the shared memory segment as if it was a file */
  int shm_fd = shm_open(shm_name, O_RDONLY, 0666);
  if (shm_fd == -1) {
    perror("shm_open: No se pudo abrir la shared memory");
    exit(EXIT_FAILURE);
  }

  /* map the shared memory segment to the address space of the process */
  struct buf_msg *shm_ptr = mmap(0, sizeof(struct buf_msg), PROT_READ, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap: No se pudo mapear la shared memory");
    close(shm_fd);
    shm_unlink(shm_name);
    exit(EXIT_FAILURE);
  }

  /*
  *  TODO
  */

  /* remove the mapped shared memory segment from the address space of the process */
  if (munmap(shm_ptr, sizeof(struct buf_msg)) == -1) {
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

  //Ver si lo hace el vista o aplicacion
  /* remove the shared memory segment from the file system */
  if (shm_unlink(shm_name) == -1) {
    perror("shm_unlink: No se pudo cerrar la shared memory");
    exit(EXIT_FAILURE);
  }

  return 0;
}