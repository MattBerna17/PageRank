#include "utilities_pagerank.h"
#define HERE __FILE__, __LINE__
#define QUI __LINE__, __FILE__


void *manage_signal(void *arg) {
    signal_info* info = (signal_info *) arg;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2); // when the pagerank computation has ended, the main thread sends a SIGUSR2 signal to the signal manager thread
    int s = 0;
    while (!(*info->terminated)) {
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
            fprintf(stderr, "Current number of iterations: %d\n", *info->numiter);
            fprintf(stderr, "Node with maximum pagerank value: %d %f\n", index, max_pagerank);
        } else if (s == SIGUSR2 && *info->terminated) {
            break;
        }
    }
    return 0;
}





void *manage_edges(void *arg) {
    input_info *info = (input_info *)arg;
    while (true) {
        xpthread_mutex_lock(info->mutex, QUI);
        while((*info->available) == 0) {
            xpthread_cond_wait(info->canread, info->mutex, QUI);
        }
        // at least 1 available edge in arr
        edge *curr_edge = info->arr[(*info->position) % info->n];
        (*info->available)--;
        (*info->position)++;
        // check on the edge
        if (curr_edge == NULL) {
            // reached end of file, terminate thread execution
            free(curr_edge);
            xpthread_cond_signal(info->canwrite, QUI);
            xpthread_mutex_unlock(info->mutex, QUI);
            break;
        } else {
            // if the edge is from a node to itself, continue
            if (curr_edge->src != curr_edge->dest) {
                // else, add the node in the inmap structure
                // check whether the edge is legal or not, else terminate the program
                if (curr_edge->src < 0 || curr_edge->dest > info->g->N - 1) {
                    printerr("[ERROR]: edge source or destination out of legal range. Terminating.", HERE);
                }
                bool added = add(&info->g->in[curr_edge->dest], curr_edge->src);
                // if succeded to add, increment number of out edges from src
                if (added) {
                    info->g->out[curr_edge->src]++;
                }
            }
            free(curr_edge);
            xpthread_cond_signal(info->canwrite, QUI);
            xpthread_mutex_unlock(info->mutex, QUI);
        }
    }
    return 0; // not using pthread_exit(NULL) since valgrind recognizes it as "still reachable"
}


double aux(graph *g, double *x, inmap *n) {
    if ((*n) == NULL) {
        return 0;
    } else if (g->out[(*n)->val] > 0) {
        return x[(*n)->val]/g->out[(*n)->val] + aux(g, x, &(*n)->left) + aux(g, x, &(*n)->right);
    }
    return 0.0;
}


double compute_y(graph *g, double *x, int j, double d) {
    double y = 0;
    inmap *n = &(g->in[j]);
    y += aux(g, x, n);
    return y;
}




/**
 * @brief Function to compute the deadend contribute
 * 
 * @param info data passed by the main thread
 */
void compute_st(compute_info *info) {
    xpthread_mutex_lock(info->de_mutex, QUI);
    while (true) {
        // wait on the first barrier until the main thread starts the next phase or until the computation has ended
        while (*info->idx_de == info->g->N && !(*info->terminated)) {
            xpthread_cond_wait(info->barrier1, info->de_mutex, QUI);
        }
        // if the deadend has been computed or the overall computation has ended, unlock and exit the function
        if (*info->is_de_computed || *info->terminated) {
            xpthread_mutex_unlock(info->de_mutex, QUI);
            return;
        }
        // if the current node is a deadend, add its x value to the overall st value
        if (info->g->out[*info->idx_de % info->g->N] == 0) {
            *info->st += info->x[*info->idx_de % info->g->N];
        }
        (*info->idx_de)++; // increment the value
        // signal if the analyzed element was the last node
        if (*info->idx_de == info->g->N) {
            xpthread_cond_signal(info->de_completed, QUI);
        }
    }
    xpthread_mutex_unlock(info->de_mutex, QUI);
}


/**
 * @brief Function to compute a pagerank iteration
 * 
 * @param info data passed by the main thread
 */
void compute_pr(compute_info *info) {
    xpthread_mutex_lock(info->pr_mutex, QUI);
    while (true) {
        // wait on the second barrier until the main thread signals the start of the next phase or the computation has ended
        while (*info->idx_pr == info->g->N && !(*info->terminated)) {
            xpthread_cond_wait(info->barrier2, info->pr_mutex, QUI);
        }
        // if the iteration has finished or the computation has finished, exit the function
        if (*info->is_pr_computed || *info->terminated) {
            xpthread_mutex_unlock(info->pr_mutex, QUI);
            return;
        }
        // compute the y element
        double sum_y = compute_y(info->g, info->x, (*info->idx_pr) % info->g->N, info->d);
        // compute xnext value
        info->xnext[(*info->idx_pr) % info->g->N] = info->teleport + info->d * sum_y + (*info->st);
        (*info->idx_pr)++; // increment the index of the pagerank
        // if the computed element was the last, signal the end of the iteration
        if (*info->idx_pr == info->g->N) {
            xpthread_cond_signal(info->pr_completed, QUI);
        }
    }
    xpthread_mutex_unlock(info->pr_mutex, QUI);
}


/**
 * @brief Function to compute the error of the current iteration
 * 
 * @param info data passed by the main thread
 */
void compute_error(compute_info *info) {
    xpthread_mutex_lock(info->error_mutex, QUI);
    while (true) {
        // wait on the final barrier until the main thread signals the end of the current iteration or the computation has ended
        while (*info->idx_error == info->g->N && !(*info->terminated)) {
            xpthread_cond_wait(info->barrier3, info->error_mutex, QUI);
        }
        // if the error has been calculated or the computation has finished, exit the function
        if (*info->is_error_computed || *info->terminated) {
            xpthread_mutex_unlock(info->error_mutex, QUI);
            return;
        }
        // compute the error for the current index
        *info->error += fabs(info->x[(*info->idx_error) % info->g->N] - info->xnext[(*info->idx_error) % info->g->N]);
        (*info->idx_error)++; // increment the index for the error calculation
        // if the element analyzed was the last, signal the end of the error computation
        if (*info->idx_error == info->g->N) {
            xpthread_cond_signal(info->error_completed, QUI);
        }
    }
    xpthread_mutex_unlock(info->error_mutex, QUI);
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








double *pagerank(graph *g, double d, double eps, int maxiter, int taux, int *numiter) {
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

        xpthread_create(&threads[i], NULL, &tbody, &infos[i], QUI);
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
    xpthread_create(sig_manager, NULL, &manage_signal, sig_info, QUI);



    // while the error is greater than the epsilon specified and the number of iteration is less than the specified one maxiter...
    while (!terminated) {
        xpthread_mutex_lock(&de_mutex, QUI);
        // wait until the St component has been calculated
        while (idx_de < g->N) {
            xpthread_cond_wait(&de_completed, &de_mutex, QUI);
        }
        // prepare for the next phase
        idx_de = 0;
        idx_pr = 0;
        is_de_computed = true;
        is_pr_computed = false;
        st = (d / g->N) * st;
        // notify the start of the second phase (pagerank computation)
        xpthread_cond_broadcast(&barrier1, QUI);
        xpthread_mutex_unlock(&de_mutex, QUI);

        xpthread_mutex_lock(&pr_mutex, QUI);
        // wait until the Xnext array has been computed
        while (idx_pr < g->N) {
            xpthread_cond_wait(&pr_completed, &pr_mutex, QUI);
        }
        // prepare for the next phase 
        idx_error = 0;
        idx_pr = 0;
        is_pr_computed = true;
        is_error_computed = false;
        // notify the start of the third phase (error computation)
        xpthread_cond_broadcast(&barrier2, QUI);
        xpthread_mutex_unlock(&pr_mutex, QUI);

        xpthread_mutex_lock(&error_mutex, QUI);
        // wait until the error has been calculated
        while (idx_error < g->N) {
            xpthread_cond_wait(&error_completed, &error_mutex, QUI);
        }
        // prepare for the next iteration
        idx_de = 0;
        idx_error = 0;
        is_error_computed = true;
        is_de_computed = false;
        (*numiter)++;
        // check if the computation has to end
        if (error <= eps || (*numiter) >= maxiter) {
            terminated = true;
            // wake up any remaining threads waiting on conditions
            xpthread_cond_broadcast(&barrier1, QUI);
            xpthread_cond_broadcast(&barrier2, QUI);
            xpthread_cond_broadcast(&barrier3, QUI);
        }

        for (int i = 0; i < g->N; i++) {
            x[i] = xnext[i];
        }
        st = 0;
        error = 0;
        // notify the start of the next iteration
        xpthread_cond_broadcast(&barrier3, QUI);
        xpthread_mutex_unlock(&error_mutex, QUI);
    }

    for (int i = 0; i < taux; i++) {
        xpthread_join(threads[i], NULL, QUI);
    }

    pthread_kill(*sig_manager, SIGUSR2); // signal the end of the pagerank computation
    xpthread_join(*sig_manager, NULL, QUI);

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


