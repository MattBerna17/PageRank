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


bool add(node** l, int val) {
    node* current = *l;
    node* prev = NULL;

    // find the position where to add the new node
    while (current != NULL && current->val > val) {
        prev = current;
        current = current->next;
    }

    // if the value is in the list, return false
    if (current != NULL && current->val == val) {
        return false;
    }

    node* new_node = (node*)malloc(sizeof(node));
    if (new_node == NULL) {
        printerr("[ERROR] malloc not succeded. Terminating.", HERE);
    }
    new_node->val = val;
    new_node->next = current;

    // if prev is null, the new node is the head of the list
    if (prev == NULL) {
        *l = new_node;
    } else {
        prev->next = new_node;
    }

    return true;
}




void clear(node* l) {
    if (l == NULL) {
        return;
    } else {
        clear(l->next);
        free(l);
    }
}