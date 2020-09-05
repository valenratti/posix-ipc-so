#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define READ 0
#define WRITE 1

void miniGrep(char *filename);

int main(int argc, char *argv[]) {
  int cant;
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  for (cant = 1; cant < argc; cant++)
    miniGrep(argv[cant]);

  while ((linelen = getline(&line, &linecap, stdin)) > 0) {
    if (line[linelen - 1] == '\n') {
      line[linelen - 1] = '\0';
    }
    miniGrep(line);
  }

  exit(EXIT_SUCCESS);
}

void miniGrep(char *filename) {
  FILE *stream;
  char output[1024] = {}, msg[1024] = {}, cmd[1024] = {};

  sprintf(cmd, "minisat %s | grep -o -e \"Number of .*[0-9]\\+\" -e \"CPU time.*\" -e \".*SATISFIABLE\"", filename);
  stream = popen(cmd, "r");
  if (stream == NULL) {
    printf("popen minisat\n");
    exit(EXIT_FAILURE);
  }
  sprintf(msg, "Filename:\t%s\n", filename);
  while (fgets(output, 1024, (FILE *)stream) != NULL) {
    strcat(msg, output);
  }
  printf("%s", msg);
  printf("PID:\t%d\n", getpid());
  pclose(stream);
}