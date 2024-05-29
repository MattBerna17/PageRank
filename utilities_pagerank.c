#include "utilities_pagerank.h"
#define HERE __LINE__, __FILE__


void *manage_signal(void *arg) {
    signal_info* info = (signal_info *) arg;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2); // when the pagerank computation has ended, the main thread sends a SIGUSR2 signal to the signal manager thread
    int s = 0;
    while (!(*info->terminated)) {
        // wait until SIGUSR1 or SIGUSR2 arrives
        int e = sigwait(&mask, &s);
        if (e != 0) printerr("[ERROR]: error during signal management. Terminating", HERE);
        if (s == SIGUSR1) {
            int index = 0;
            double max_pagerank = info->x[index];
            for (int i = 0; i < info->n; i++) {
                if (info->x[i] > max_pagerank) {
                    max_pagerank = info->x[i];
                    index = i;
                }
            }
            fprintf(stderr, "\nCurrent number of iterations: %d\n", *info->numiter);
            fprintf(stderr, "Node with maximum pagerank value: %d %f\n\n", index, max_pagerank);
        } else if (s == SIGUSR2 && *info->terminated) {
            break;
        }
    }
    return 0;
}





void *manage_edges(void *arg) {
    input_info *info = (input_info *)arg;
    
    while (true) {
        xpthread_mutex_lock(info->mutex, HERE);
        
        while((*info->available) == 0) {
            xpthread_cond_wait(info->canread, info->mutex, HERE);
        }
        
        // at least 1 available edge in arr
        edge *curr_edge = info->arr[(*info->position) % info->n];
        (*info->available)--;
        (*info->position)++;
        
        // check on the edge
        if (curr_edge == NULL) {
            // reached end of file, terminate thread execution
            free(curr_edge);
            xpthread_cond_signal(info->canwrite, HERE);
            xpthread_mutex_unlock(info->mutex, HERE);
            break;
        } else {
            // if the edge is from a node to itself, continue
            if (curr_edge->src != curr_edge->dest) {
                // check whether the edge is legal or not, else terminate the program
                if (curr_edge->src < 0 || curr_edge->dest > info->g->N - 1) {
                    printerr("[ERROR]: edge source or destination out of legal range. Terminating.", HERE);
                }
                
                // add the node to the tree
                bool added = add(&info->g->in[curr_edge->dest], curr_edge->src);
                
                // if succeeded to add, increment number of out edges from src
                if (added) {
                    info->g->out[curr_edge->src]++;
                }
            }
            free(curr_edge);
            xpthread_cond_signal(info->canwrite, HERE);
            xpthread_mutex_unlock(info->mutex, HERE);
        }
    }
    return 0; // not using pthread_exit(NULL) since valgrind recognizes it as "still reachable"
}



/**
 * @brief Function to iteratively compute the sum of Y_i for each node i in IN(j)
 * 
 * @param info data passed by the main thread
 */
double compute_sum_y(grafo *g, double *y, int j) {
    double sum = 0.0;
    node *current = g->in[j];

    node **stack = malloc(sizeof(node*) * g->N); // stack of nodes to sum
    if (stack == NULL) {
        printerr("[ERROR] malloc failed. Terminating.", HERE);
    }
    int top = -1; // index to the top of the stack

    // push the current node to the top of the stack
    if (current != NULL) {
        top++;
        stack[top] = current;
    }

    // while the stack is not empty...
    while (top >= 0) {
        current = stack[top]; // pop node from the stack
        top--;
        // add the Y value of the node
        sum += y[current->val];

        // push left and right to the stack
        if (current->right != NULL) {
            top++;
            stack[top] = current->right;
        }
        if (current->left != NULL) {
            top++;
            stack[top] = current->left;
        }
    }

    free(stack);
    return sum;
}


/**
 * @brief Function to compute the Y array
 * 
 * @param info data passed by the main thread
 */
void compute_y(compute_info *info) {
    xpthread_mutex_lock(info->y_mutex, HERE);
    while (true) {
        // wait on the first barrier until the main thread starts the next phase or until the computation has ended
        while (*info->idx_y == info->g->N && !(*info->terminated)) {
            xpthread_cond_wait(info->barrier0, info->y_mutex, HERE);
        }
        // if the deadend has been computed or the overall computation has ended, unlock and exit the function
        if (*info->is_y_computed || *info->terminated) {
            xpthread_mutex_unlock(info->y_mutex, HERE);
            return;
        }

        // compute Y_i
        info->y[(*info->idx_y) % info->g->N] = info->x[(*info->idx_y) % info->g->N]/info->g->out[(*info->idx_y) % info->g->N];

        (*info->idx_y)++; // increment the value
        // signal if the analyzed element was the last node
        if (*info->idx_y == info->g->N) {
            xpthread_cond_signal(info->y_completed, HERE);
        }
    }
    xpthread_mutex_unlock(info->y_mutex, HERE);
}



/**
 * @brief Function to compute the deadend contribute
 * 
 * @param info data passed by the main thread
 */
void compute_st(compute_info *info) {
    xpthread_mutex_lock(info->de_mutex, HERE);
    while (true) {
        // wait on the first barrier until the main thread starts the next phase or until the computation has ended
        while (*info->idx_de == info->g->N && !(*info->terminated)) {
            xpthread_cond_wait(info->barrier1, info->de_mutex, HERE);
        }
        // if the deadend has been computed or the overall computation has ended, unlock and exit the function
        if (*info->is_de_computed || *info->terminated) {
            xpthread_mutex_unlock(info->de_mutex, HERE);
            return;
        }
        // if the current node is a deadend, add its x value to the overall st value
        if (info->g->out[*info->idx_de % info->g->N] == 0) {
            *info->st += info->x[*info->idx_de % info->g->N];
        }
        (*info->idx_de)++; // increment the value
        // signal if the analyzed element was the last node
        if (*info->idx_de == info->g->N) {
            xpthread_cond_signal(info->de_completed, HERE);
        }
    }
    xpthread_mutex_unlock(info->de_mutex, HERE);
}


/**
 * @brief Function to compute a pagerank iteration
 * 
 * @param info data passed by the main thread
 */
void compute_pr(compute_info *info) {
    xpthread_mutex_lock(info->pr_mutex, HERE);
    while (true) {
        // wait on the second barrier until the main thread signals the start of the next phase or the computation has ended
        while (*info->idx_pr == info->g->N && !(*info->terminated)) {
            xpthread_cond_wait(info->barrier2, info->pr_mutex, HERE);
        }
        // if the iteration has finished or the computation has finished, exit the function
        if (*info->is_pr_computed || *info->terminated) {
            xpthread_mutex_unlock(info->pr_mutex, HERE);
            return;
        }
        // compute the y element
        double sum_y = compute_sum_y(info->g, info->y, (*info->idx_pr) % info->g->N);
        // compute xnext value
        info->xnext[(*info->idx_pr) % info->g->N] = info->teleport + info->d * sum_y + (*info->st);
        (*info->idx_pr)++; // increment the index of the pagerank
        // if the computed element was the last, signal the end of the iteration
        if (*info->idx_pr == info->g->N) {
            xpthread_cond_signal(info->pr_completed, HERE);
        }
    }
    xpthread_mutex_unlock(info->pr_mutex, HERE);
}


/**
 * @brief Function to compute the error of the current iteration
 * 
 * @param info data passed by the main thread
 */
void compute_error(compute_info *info) {
    xpthread_mutex_lock(info->error_mutex, HERE);
    while (true) {
        // wait on the final barrier until the main thread signals the end of the current iteration or the computation has ended
        while (*info->idx_error == info->g->N && !(*info->terminated)) {
            xpthread_cond_wait(info->barrier3, info->error_mutex, HERE);
        }
        // if the error has been calculated or the computation has finished, exit the function
        if (*info->is_error_computed || *info->terminated) {
            xpthread_mutex_unlock(info->error_mutex, HERE);
            return;
        }
        // compute the error for the current index
        *info->error += fabs(info->x[(*info->idx_error) % info->g->N] - info->xnext[(*info->idx_error) % info->g->N]);
        (*info->idx_error)++; // increment the index for the error calculation
        // if the element analyzed was the last, signal the end of the error computation
        if (*info->idx_error == info->g->N) {
            xpthread_cond_signal(info->error_completed, HERE);
        }
    }
    xpthread_mutex_unlock(info->error_mutex, HERE);
}

/**
 * @brief Function executed by each thread
 * 
 * @param arg data passed by the main thread
 * @return void* 
 */
void *tbody(void *arg) {
    compute_info *info = (compute_info *) arg;
    // repeat these 3 computations until the main signals the end of the computation
    while (!(*info->terminated)) {
        // zero phase: compute Y_i for the current iteration
        compute_y(info);

        // zero barrier

        // first phase: compute St for the current iteration
        compute_st(info);

        // first barrier
        
        // second phase: compute Xnext for the current iteration
        compute_pr(info);
        
        // second barrier

        // third phase: compute the error for the current iteration
        compute_error(info);

        // third barrier
    }
    return 0;
}








double *pagerank(grafo *g, double d, double eps, int maxiter, int taux, int *numiter) {
    // define the 3 used arrays: x, y and xnext
    double *x = malloc(sizeof(double) * g->N);
    if (x == NULL) {
        printerr("[ERROR]: malloc not succeded. Terminating", HERE);
    }
    double *y = malloc(sizeof(double) * g->N);
    if (y == NULL) {
        printerr("[ERROR]: malloc not succeded. Terminating", HERE);
    }
    double *xnext = malloc(sizeof(double) * g->N);
    if (xnext == NULL) {
        printerr("[ERROR]: malloc not succeded. Terminating", HERE);
    }

    // inizialization of the probability array X(1)
    for (int i = 0; i < g->N; i++) {
        x[i] = 1.0/g->N;
    }
    double teleport = (1.0 - d)/g->N; // teleporting term
    
    // initializing info to pass to the threads for the communication
    pthread_t *threads = malloc(sizeof(pthread_t) * taux);
    if (threads == NULL) {
        printerr("[ERROR]: malloc not succeded. Terminating", HERE);
    }
    compute_info *infos = malloc(sizeof(compute_info) * taux);
    if (infos == NULL) {
        printerr("[ERROR]: malloc not succeded. Terminating", HERE);
    }
    
    // create the condition variables, mutex and variables needed to communicate with the threads
    bool terminated = false; // pagerank computation terminated?

    pthread_mutex_t de_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t de_completed = PTHREAD_COND_INITIALIZER;
    pthread_cond_t barrier1 = PTHREAD_COND_INITIALIZER;
    bool is_de_computed = false;
    double st = 0.0;
    int idx_de = 0;

    pthread_mutex_t y_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t y_completed = PTHREAD_COND_INITIALIZER;
    pthread_cond_t barrier0 = PTHREAD_COND_INITIALIZER;
    bool is_y_computed = false;
    int idx_y = 0;

    pthread_mutex_t pr_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t pr_completed = PTHREAD_COND_INITIALIZER;
    pthread_cond_t barrier2 = PTHREAD_COND_INITIALIZER;
    bool is_pr_computed = false;
    int idx_pr = 0;

    pthread_mutex_t error_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t error_completed = PTHREAD_COND_INITIALIZER;
    pthread_cond_t barrier3 = PTHREAD_COND_INITIALIZER;
    bool is_error_computed = false;
    double error = 0.0;
    int idx_error = 0;


    // set up information for the auxiliary threads
    for (int i = 0; i < taux; i++) {
        infos[i].terminated = &terminated;
        infos[i].g = g;

        infos[i].y_mutex = &y_mutex;
        infos[i].y_completed = &y_completed;
        infos[i].barrier0 = &barrier0;
        infos[i].is_y_computed = &is_y_computed;
        infos[i].idx_y = &idx_y;

        infos[i].de_mutex = &de_mutex;
        infos[i].de_completed = &de_completed;
        infos[i].barrier1 = &barrier1;
        infos[i].is_de_computed = &is_de_computed;
        infos[i].st = &st;
        infos[i].idx_de = &idx_de;

        infos[i].pr_mutex = &pr_mutex;
        infos[i].pr_completed = &pr_completed;
        infos[i].barrier2 = &barrier2;
        infos[i].is_pr_computed = &is_pr_computed;
        infos[i].x = x;
        infos[i].y = y;
        infos[i].xnext = xnext;
        infos[i].idx_pr = &idx_pr;
        infos[i].teleport = teleport;
        infos[i].d = d;

        infos[i].error_mutex = &error_mutex;
        infos[i].error_completed = &error_completed;
        infos[i].barrier3 = &barrier3;
        infos[i].is_error_computed = &is_error_computed;
        infos[i].error = &error;
        infos[i].idx_error = &idx_error;

        xpthread_create(&threads[i], NULL, &tbody, &infos[i], HERE);
    }


    
    // create the SIGUSR1 manager thread
    signal_info *sig_info = malloc(sizeof(signal_info));
    if (sig_info == NULL) {
        printerr("[ERROR]: malloc not succeded. Terminating", HERE);
    }
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1); // add SIGUSR1 to the list of blocked signals
    sigaddset(&mask, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &mask, NULL); // block SIGUSR1 for this thread (main thread)
    // create the signal manager thread
    pthread_t *sig_manager = malloc(sizeof(pthread_t));
    if (sig_manager == NULL) {
        printerr("[ERROR]: malloc not succeded. Terminating", HERE);
    }
    // pass the data to the signal manager
    sig_info->numiter = numiter;
    sig_info->x = x;
    sig_info->terminated = &terminated;
    sig_info->n = g->N;
    xpthread_create(sig_manager, NULL, &manage_signal, sig_info, HERE);



    // while the error is greater than the epsilon specified and the number of iteration is less than the specified one maxiter...
    while (!terminated) {
        xpthread_mutex_lock(&y_mutex, HERE);
        // wait until the Y array has been calculated
        while (idx_y < g->N) {
            xpthread_cond_wait(&y_completed, &y_mutex, HERE);
        }
        // prepare for the next phase
        idx_y = 0;
        idx_de = 0;
        is_y_computed = true;
        is_de_computed = false;
        // notify the start of the first phase (St computation)
        xpthread_cond_broadcast(&barrier0, HERE);
        xpthread_mutex_unlock(&y_mutex, HERE);

        xpthread_mutex_lock(&de_mutex, HERE);
        // wait until the St component has been calculated
        while (idx_de < g->N) {
            xpthread_cond_wait(&de_completed, &de_mutex, HERE);
        }
        // prepare for the next phase
        idx_de = 0;
        idx_pr = 0;
        is_de_computed = true;
        is_pr_computed = false;
        st = (d / g->N) * st;
        // notify the start of the second phase (pagerank computation)
        xpthread_cond_broadcast(&barrier1, HERE);
        xpthread_mutex_unlock(&de_mutex, HERE);

        xpthread_mutex_lock(&pr_mutex, HERE);
        // wait until the Xnext array has been computed
        while (idx_pr < g->N) {
            xpthread_cond_wait(&pr_completed, &pr_mutex, HERE);
        }
        // prepare for the next phase 
        idx_error = 0;
        idx_pr = 0;
        is_pr_computed = true;
        is_error_computed = false;
        // notify the start of the third phase (error computation)
        xpthread_cond_broadcast(&barrier2, HERE);
        xpthread_mutex_unlock(&pr_mutex, HERE);

        xpthread_mutex_lock(&error_mutex, HERE);
        // wait until the error has been calculated
        while (idx_error < g->N) {
            xpthread_cond_wait(&error_completed, &error_mutex, HERE);
        }
        // prepare for the next iteration
        idx_y = 0;
        idx_error = 0;
        is_error_computed = true;
        is_y_computed = false;
        (*numiter)++;

        // check if the computation has to end
        if (error <= eps || (*numiter) >= maxiter) {
            terminated = true;
            // wake up any remaining threads waiting on conditions
            xpthread_cond_broadcast(&barrier0, HERE);
            xpthread_cond_broadcast(&barrier1, HERE);
            xpthread_cond_broadcast(&barrier2, HERE);
            xpthread_cond_broadcast(&barrier3, HERE);
        }

        for (int i = 0; i < g->N; i++) {
            x[i] = xnext[i];
        }
        st = 0;
        error = 0;
        // notify the start of the next iteration
        xpthread_cond_broadcast(&barrier3, HERE);
        xpthread_mutex_unlock(&error_mutex, HERE);
    }

    for (int i = 0; i < taux; i++) {
        xpthread_join(threads[i], NULL, HERE);
    }

    pthread_kill(*sig_manager, SIGUSR2); // signal the end of the pagerank computation
    xpthread_join(*sig_manager, NULL, HERE);

    // free used data structures
    free(threads);
    free(infos);
    free(x);
    free(y);
    free(sig_info);
    free(sig_manager);

    return xnext;
}





int cmp_ranks(const void *a, const void *b) {
    rank **r1 = (rank **) a;
    rank **r2 = (rank **) b;
    if ((*r1)->val < (*r2)->val) return 1;
    else if ((*r1)->val > (*r2)->val) return -1;
    else return 0;
}


