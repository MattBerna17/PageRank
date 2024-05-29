#include "helpers.h"
#define Buf_size 32

// ----------------------------------  DATA STRUCTURES  ----------------------------------
/**
 * @brief Data structure to store a tree in the graph
 * 
 */
typedef struct inmap {
    node *list;
    pthread_mutex_t *list_mutex;
} inmap;


/**
 * @brief Data structure to implement the adjacent graph
 * @see inmap
 * 
 */
typedef struct graph {
    int N;          // number of nodes in the graph
    int *out;       // array with the number of exiting edges for each node
    inmap *in;     // array of entering edges for each node
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


/**
 * @brief Data structure to pass info to the threads that need to compute the pagerank calculation
 * 
 */
typedef struct compute_info {
    bool *terminated;
    graph *g;

    pthread_mutex_t *y_mutex;
    pthread_cond_t *y_completed;
    pthread_cond_t *barrier0;
    bool *is_y_computed;
    // double *y;
    int *idx_y;

    pthread_mutex_t *de_mutex;
    pthread_cond_t *de_completed;
    pthread_cond_t *barrier1;
    bool *is_de_computed;
    double *st;
    int *idx_de;

    pthread_mutex_t *pr_mutex;
    pthread_cond_t *pr_completed;
    pthread_cond_t *barrier2;
    bool *is_pr_computed;
    double *x;
    double *xnext;
    double *y;
    int *idx_pr;
    double teleport;
    double d;

    pthread_mutex_t *error_mutex;
    pthread_cond_t *error_completed;
    pthread_cond_t *barrier3;
    bool *is_error_computed;
    double *error;
    int *idx_error;
} compute_info;


/**
 * @brief Data structure to pass info to the signal manager thread
 * 
 */
typedef struct signal_info {
    double *x;          // array containing pagerank values
    int n;              // length of X
    int *numiter;       // current number of iterations
    bool *terminated;   // manage signals until the value is true
} signal_info;


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
 * @brief Function executed by the signal manager thread
 * 
 * @param arg signal_info struct to receive data from the main thread
 * @return void* 
 */
void *manage_signal(void *arg);



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
