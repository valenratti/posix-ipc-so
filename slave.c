#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(){
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, stdin)) > 0) {
        printf("Mensaje del hijo %d a traves del pipe para my padre\n", getpid());
        fwrite(line, linelen, 1, stdout);
    }
    exit(EXIT_SUCCESS);

}