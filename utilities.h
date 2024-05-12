#include "xerrori.h"


// ----------------------------------  DATA STRUCTURES  ----------------------------------
/**
 * Struct to implement the array of edges
 * (Linked list maybe)
*/
typedef struct inmap {
    // 
} inmap;


/**
 * Struct to describe the oriented graph
*/
typedef struct graph {
    int N;      // number of nodes in the graph
    int *out;   // array with the number of exiting edges for each node
    inmap *in;  // array of entering edges for each node
} graph;



// ----------------------------------  FUNCTIONS  ----------------------------------
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


void hello();
