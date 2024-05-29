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



bool add(node** t, int val) {
    // create the node for the new value
    node *new_node = (node*) malloc(sizeof(node));
    new_node->val = val;
    new_node->left = NULL;
    new_node->right = NULL;
    pthread_mutex_init(&(new_node->lock), NULL);

    // if the tree is empty, place the node as the root node
    if (*t == NULL) {
        *t = new_node;
        return true;
    }

    node* current = *t;
    node* parent = NULL;

    pthread_mutex_lock(&(current->lock));

    while (current != NULL) {
        parent = current;

        if (val < current->val) {
            if (current->left != NULL) {
                pthread_mutex_lock(&(current->left->lock));
                pthread_mutex_unlock(&(current->lock));
                current = current->left;
            } else {
                current->left = new_node;
                pthread_mutex_unlock(&(current->lock));
                return true;
            }
        } else if (val > current->val) {
            if (current->right != NULL) {
                pthread_mutex_lock(&(current->right->lock));
                pthread_mutex_unlock(&(current->lock));
                current = current->right;
            } else {
                current->right = new_node;
                pthread_mutex_unlock(&(current->lock));
                return true;
            }
        } else {
            // if the value is already in the tree, return false
            free(new_node);
            pthread_mutex_unlock(&(current->lock));
            return false;
        }
    }
    return false;
}






void clear(node* t) {
    if (t == NULL) {
        return;
    } else {
        clear(t->left);
        clear(t->right);
        free(t);
    }
}