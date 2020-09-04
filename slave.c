#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define READ 0
#define WRITE 1

int main(int argc, char *argv[]) {
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  pid_t minisat_slave;
  FILE *stream_ms, *stream_grep;
  char output[4096], ret_output[4096];

  //while ((linelen = getline(&line, &linecap, stdin)) > 0) {
  int mypipe_read[2], mypipe_write[2];
  pipe(mypipe_read);
  pipe(mypipe_write);
  minisat_slave = fork();
  if (minisat_slave == -1) {
    perror("fork\n");
    exit(EXIT_FAILURE);
  } else if (minisat_slave == 0) {
    close(mypipe_read[WRITE]);
    close(mypipe_write[READ]);
    execlp("/bin/grep", "grep", "-o", "-e", "\"Number of.*[0-9]\\+\"", "-e", "\"CPU time.*\"", "-e", ".*SATISFIABLE", (char *)NULL);
    exit(EXIT_FAILURE);
  } else {
    close(mypipe_read[READ]);
    close(mypipe_write[WRITE]);
    stream_ms = popen("minisat CNF/hole6.cnf", "r");
    if (stream_ms == NULL) {
      printf("popen minisat\n");
      exit(EXIT_FAILURE);
    }
    while (fgets(output, 4096, (FILE *)stream_ms) != NULL) {
      printf("%s", output);
      if (write(mypipe_read[WRITE], output, 4096) == -1) {
        printf("Fallo\n");
        exit(1);
      }
    }
    printf("hola\n");
    pclose(stream_ms);
    close(mypipe_read[WRITE]);
    close(mypipe_write[READ]);
  }
  // }
  exit(EXIT_SUCCESS);
}