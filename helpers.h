#include "xerrori.h"
#include "math.h"

// ---------------------------------- DATA STRUCTURES ----------------------------------
/**
 * @brief Data structure to represent the node of an integer linked list
 * 
 */
typedef struct node {
    int val;                // index of the node
    struct node* next;      // pointer to the next node
} node;


// ---------------------------------- FUNCTIONS ----------------------------------
/**
 * @brief Helper function to print errors
 * 
 * @param msg message to print
 * @param line line that caused the error
 * @param file file containing the error
 */
void printerr(char *msg, int line, char *file);

/**
 * @brief Function to read a line from the specified file
 * 
 * @param line      pointer to the string to store the line
 * @param length    dimension of the string
 * @param f         file to read the line from
 * @return int      1 if data has been read from the file. 0 if file has ended. -1 if an error occurred
 */
int read_line(char** line, size_t *length, FILE *f);

/**
 * @brief Function to add a value in the linked list l
 * 
 * @param l         head of the linked list
 * @param val       value to add to the list
 * @return true     if the value is not present in the list (and the function added it)
 * @return false    if the value was already in the list
 */
bool add(node** l, int val);

/**
 * @brief Function to free the list
 * 
 * @param l list to free
 */
void clear(node* l);

