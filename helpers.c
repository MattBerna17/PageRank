#include "helpers.h"
#define HERE __LINE__, __FILE__


void printerr(char *msg, int line, char *file) {
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


bool add(node** t, int val) {
    // Allocazione di memoria per il nuovo nodo
    node* new_node = (node*)malloc(sizeof(node));
    if (new_node == NULL) {
        // Se l'allocazione della memoria fallisce, ritorna false
        return false;
    }

    // Assegna il valore al nuovo nodo
    new_node->val = val;
    // Imposta il puntatore next del nuovo nodo al primo nodo corrente della lista
    new_node->next = *t;
    // Aggiorna il puntatore t per puntare al nuovo nodo
    *t = new_node;

    // Ritorna true per indicare che l'operazione è andata a buon fine
    return true;
}




void clear(node* t) {
    if (t == NULL) {
        return;
    } else {
        clear(t->next);
        free(t);
    }
}