#include "xerrori.h"
#include "math.h"

// ---------------------------------- DATA STRUCTURES ----------------------------------
/**
 * @brief Data structure to represent the node of an integer tree
 * 
 */
typedef struct node {
    int val;                // index of the node
    struct node* next;      // pointer to the left child
} node;


// ---------------------------------- FUNCTIONS ----------------------------------
/**
 * @brief Helper function to print errors
 * 
 * @param msg message to print
 * @param file file containing the error
 * @param line line that caused the error
 */
void printerr(char *msg, int line, char *file);

/**
 * @brief Function to read a line from the specified file
 * 
 * @param line      pointer to the char array to store the string
 * @param length    dimension of the string
 * @param f         file to read the line from
 * @return int      1 if data has been read from the file. 0 if file has ended. -1 if an error occurred
 */
int read_line(char** line, size_t *length, FILE *f);

/**
 * @brief Function to add a value in the t tree (binary search tree)
 * 
 * @param t         tree to add the value to
 * @param val       value to add to the tree
 * @return true     if the value is not present in the tree (and the function added it)
 * @return false    if the value was already in the tree
 */
bool add(node** t, int val);

/**
 * @brief Function to free the tree
 * 
 * @param t tree to dealloc
 */
void clear(node* t);

