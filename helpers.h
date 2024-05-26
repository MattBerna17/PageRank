#include "xerrori.h"
#include "math.h"

/**
 * @brief Data structure to represent the node of an integer tree
 * 
 */
typedef struct node {
    int val;               // index of the node
    struct node* left;     // pointer to the left child
    struct node* right;    // pointer to the right child
} node;



/**
 * @brief Helper function to print errors
 * 
 * @param msg message to print
 * @param file file containing the error
 * @param line line that caused the error
 */
void printerr(char *msg, char *file, int line);


int read_line(char** line, size_t *length, FILE *f);


bool add(node** t, int val);

void clear(node* t);

