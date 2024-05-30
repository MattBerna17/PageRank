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
    node* current = *t;
    node* prev = NULL;

    // Scorriamo la lista per trovare la posizione corretta per inserire l'elemento
    while (current != NULL && current->val > val) {
        prev = current;
        current = current->next;
    }

    // Se troviamo un nodo con lo stesso valore, non aggiungiamo nulla
    if (current != NULL && current->val == val) {
        return false; // Elemento già presente
    }

    // Creiamo un nuovo nodo
    node* new_node = (node*)malloc(sizeof(node));
    if (new_node == NULL) {
        return false; // Errore nell'allocazione della memoria
    }
    new_node->val = val;
    new_node->next = current;

    // Se prev è NULL, significa che dobbiamo inserire il nuovo nodo all'inizio della lista
    if (prev == NULL) {
        *t = new_node;
    } else {
        prev->next = new_node;
    }

    return true; // Elemento aggiunto con successo
}




void clear(node* t) {
    if (t == NULL) {
        return;
    } else {
        clear(t->next);
        free(t);
    }
}