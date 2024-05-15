#include "helpers.h"
#define Buf_size 32

// ----------------------------------  DATA STRUCTURES  ----------------------------------
/**
 * @brief Data structure to implement the incoming nodes for each node
 * 
 */
typedef struct inmap {
    int val; // index of the node
    struct inmap* next; // pointer to the next element of the list
} inmap;


/**
 * @brief Data structure to implement the adjacent graph
 * @see inmap
 * 
 */
typedef struct graph {
    int N;      // number of nodes in the graph
    int *out;   // array with the number of exiting edges for each node
    inmap *in;  // array of entering edges for each node
} graph;


/**
 * @brief Data structure to represent edges passed between main thread and each consumer thread
 * 
 */
typedef struct edge {
    int src; // index of the source node of the edge
    int dest; // index of the destination node of the edge
} edge;


/**
 * @brief Data passed to each thread during the input file reading phase
 * @see edge
 * 
 */
typedef struct input_info {
    pthread_cond_t canread;     // cv for reading from the buffer
    pthread_cond_t canwrite;    // cv for writing to the buffer
    pthread_mutex_t mutex;      // lock for the data array
    edge **arr;                 // array to pass edges between threads
    int *available;             // number of elements in the array
    int n;                      // length of the data array
} input_info;


// ----------------------------------  FUNCTIONS  ----------------------------------
/**
 * @brief Function executed by the threads to manage edges
 * 
 * @param arg input_info struct to communicate with the main thread
 * @return void* 
 */
void *manage_edges(void *arg);



/**
 * @brief Function to calculate pagerank algorithm
 * @see graph
 * 
 * @param g         graph to calculate pagerank of 
 * @param d         damping factor
 * @param eps       tolerance
 * @param maxiter   max number of iteration (to assure the termination)
 * @param numiter   actual number of iteration
 * @return double   return array of pagerank
 */
double *pagerank(graph *g, double d, double eps, int maxiter, int *numiter);
