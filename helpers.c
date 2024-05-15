#include "helpers.h"
#define HERE __FILE__, __LINE__


void printerr(char *msg, char *file, int line) {
    fprintf(stderr, "\n===================  ERROR AT FILE %s, LINE %d  ===================\n", file, line);
    fprintf(stderr, "%s\n\n", msg);
    exit(1);
}

char getfirstchar(char *str) {
    int i = 0;
    while (true) {
        if (str[i] != ' ') {
            return str[i];
        } else if (str[i] == '\0') {
            return NULL;
        }
        i++;
    }
}


int readline(char *line, FILE *f) {
    size_t length = 0;
    int e = getline(&line, &length, f); // read one line from the file and put it in the line buffer
    if (e == -1 && errno) {
        // received an error during the read of the file
        free(line);
        printerr("[ERROR]: Bad input file read. Terminating.", HERE);
    } else if (e == -1) {
        // end of file
        return 0;
    } else {
        // line contains the line read from the file
        // parse it to see if it's a comment or contains data
        char firstchar = getfirstchar(line);
        if (firstchar == '%') {
            // the line is a comment
            // return code 0
            return 1;
        } else {
            // check if it's an edge or the "header" line (containing r c n) by counting the number of integers in the line (3 if header, 2 if edge)
            char *number;
            int counter = 0;
            number = strtok(line, " ");
            while (number) {
                counter++;
                number = strtok(NULL, " ");
            }
            if (counter == 3) {
                return 3;
            } else if (counter == 2) {
                return 2;
            } else {
                // in case of bad file format
                printerr("[ERROR]: bad file formatting, error during read. Terminating.", HERE);
            }
        }
    }
}