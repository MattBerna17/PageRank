#include "helpers.h"
#define HERE __FILE__, __LINE__


void printerr(char *msg, char *file, int line) {
    fprintf(stderr, "\n===================  ERROR AT FILE %s, LINE %d  ===================\n", file, line);
    fprintf(stderr, "%s\n\n", msg);
    exit(1);
}



int read_line(char** line, size_t *length, FILE *f) {
    int e = getline(line, length, f);
    if (e == -1 && errno) {
        // in case of error during file read
        return -1;
    } else if (e == -1) {
        // end of file
        return 0;
    } else {
        // read succeded
        return 1;
    }
}
