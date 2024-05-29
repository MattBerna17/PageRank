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
    // create the node for the new value
    node *new_node = malloc(sizeof(node));
    if (new_node == NULL) {
        printerr("[ERROR] malloc failed. Terminating.", HERE);
    }
    new_node->val = val;
    new_node->left = NULL;
    new_node->right = NULL;
    xpthread_mutex_init(&(new_node->lock), NULL, HERE);

    // if the tree is empty, place the node as the root node
    if (*t == NULL) {
        *t = new_node;
        return true;
    }

    node* current = *t;

    pthread_mutex_lock(&(current->lock));
    // else, iteratively scan the BST to find or add the value
    while (current != NULL) {
        if (val < current->val) {
            // the value goes to the left of the current node
            if (current->left != NULL) {
                // if the left node is not NULL, go to the left
                pthread_mutex_lock(&(current->left->lock));
                pthread_mutex_unlock(&(current->lock));
                current = current->left;
            } else {
                // if the left node is NULL, place it there and return true
                current->left = new_node;
                pthread_mutex_unlock(&(current->lock));
                return true;
            }
        } else if (val > current->val) {
            // the value goes to the right of the current node
            if (current->right != NULL) {
                // if the right node is not NULL, go to the right
                pthread_mutex_lock(&(current->right->lock));
                pthread_mutex_unlock(&(current->lock));
                current = current->right;
            } else {
                // if the right node is NULL, place it and return true
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