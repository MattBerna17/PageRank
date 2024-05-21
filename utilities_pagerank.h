#include "helpers.h"
#define Buf_size 32

// ----------------------------------  DATA STRUCTURES  ----------------------------------
/**
 * @brief Data structure to implement the incoming nodes for each node (binary search tree)
 * 
 */
typedef struct inmap {
    int val;                // index of the node
    struct inmap* left;     // pointer to the left child
    struct inmap* right;    // pointer to the right child
} inmap;


/**
 * @brief Data structure to implement the adjacent graph
 * @see inmap
 * 
 */
typedef struct graph {
    int N;          // number of nodes in the graph
    int *out;       // array with the number of exiting edges for each node
    inmap **in;     // array of entering edges for each node
} graph;


/**
 * @brief Data structure to represent edges passed between main thread and each consumer thread
 * 
 */
typedef struct edge {
    int src;    // index of the source node of the edge
    int dest;   // index of the destination node of the edge
} edge;


/**
 * @brief Data passed to each thread during the input file reading phase
 * @see edge
 * 
 */
typedef struct input_info {
    pthread_cond_t *canread;     // cv for reading from the buffer
    pthread_cond_t *canwrite;    // cv for writing to the buffer
    pthread_mutex_t *mutex;      // lock for the data array
    graph *g;                   // instance of the graph
    edge **arr;                 // array to pass edges between threads
    int *available;             // number of elements in the array
    int n;                      // length of the data array
    int *position;              // current position in the array
} input_info;




typedef struct compute_info {
    pthread_cond_t *main_can_proceed;
    pthread_cond_t *threads_can_proceed;
    pthread_mutex_t *mutex;
    graph *g;
    int *n_computed;        // number of elements of the X(t) array computed
    int *position;
    double teleport;
    double d;
    double *x;              // array X(t)
    double *y;              // array Y(t)
    double *xnext;          // array X(t+1)
    int n;                  // number of elements of the arrays X, Y, Xnext
    bool *start;            // boolean to start the computation
} compute_info;



/**
 * @brief Data structure to store the ranks in a tuple: (index, value). It's only used to store the top k nodes
 * 
 */
typedef struct rank {
    int index;  // index of the node
    double val; // rank of the node
} rank;




// ----------------------------------  FUNCTIONS  ----------------------------------
/**
 * @brief Function to add the value passed in the inmap
 * 
 * @param t         root node of the inmap tree
 * @param val       value to add to the inmap tree
 * @return true     if the new value is added correctly,
 * @return false    if the value was already present in the inmap tree
*/
bool add(inmap** t, int val);

/**
 * @brief Function to free the memory blocks allocated dinamically for the inmap tree
 * 
 * @param t root node of the inmap tree
 */
void clear(inmap *t);


/**
 * @brief Function executed by the threads to manage edges
 * 
 * @param arg input_info struct to communicate with the main thread
 * @return void* 
 */
void *manage_edges(void *arg);


/**
 * @brief Function to compare ranks (used for qsort call)
 * 
 * @param a first rank to compare
 * @param b second rank to compare
 */
int cmp_ranks(const void *a, const void *b);



/**
 * @brief Function to calculate pagerank algorithm
 * @see graph
 * 
 * @param g         graph to calculate pagerank of 
 * @param d         damping factor
 * @param eps       tolerance
 * @param maxiter   max number of iteration (to assure the termination)
 * @param taux      number of auxiliary threads
 * @param numiter   actual number of iteration
 * @return double   return array of pagerank
 */
double *pagerank(graph *g, double d, double eps, int maxiter, int taux, int *numiter);
