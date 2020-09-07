// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define READ 0
#define WRITE 1

void miniGrep(char *filename, char *pid);

int main(void) {
  char *line = NULL, pid_msg[15];
  size_t linecap = 0;
  ssize_t linelen;

  sprintf(pid_msg, "PID:\t%d\n", getpid());
  while ((linelen = getline(&line, &linecap, stdin)) > 0) {
    if (line[linelen - 1] == '\n') {
      line[linelen - 1] = '\0';
    }
    miniGrep(line, pid_msg);
  }
  free(line);
  exit(EXIT_SUCCESS);
}

void miniGrep(char *filename, char *pid) {
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
  pclose(stream);
  strcat(msg, pid);
  printf("%s", msg);
  fflush(stdout);
}