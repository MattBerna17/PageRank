#include "utilities_pagerank.h"
#define HERE __FILE__, __LINE__
#define QUI __LINE__, __FILE__


bool add(inmap** t, int val) {
    // create the node for the new value
    inmap* new_node = (inmap*)malloc(sizeof(inmap));
    new_node->val = val;
    new_node->left = NULL;
    new_node->right = NULL;

    // if the tree is empty, place the node as the root node
    if (*t == NULL) {
        *t = new_node;
        return true;
    }

    inmap* current = *t;
    inmap* parent = NULL;
    while (current != NULL) {
        parent = current;

        // if the value is less then the current node, go left
        if (val < current->val) {
            current = current->left;
            if (current == NULL) {
                parent->left = new_node;
                return true;
            }
        } else if (val > current->val) {
            // if the value is greater then the current node, go right
            current = current->right;
            if (current == NULL) {
                parent->right = new_node;
                return true;
            }
        } else {
            // if the value is already in the tree, return false
            free(new_node);
            return false;
        }
    }
    return false;
}




void clear(inmap *t) {
    if (t == NULL) {
        return;
    } else {
        clear(t->left);
        clear(t->right);
        free(t);
    }
}




void *manage_signal(void *arg) {
    signal_info* info = (signal_info *) arg;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
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




double compute_error(double *xnext, double *x, int n) {
    double err = 0.0;
    for (int i = 0; i < n; i++) {
        err += fabs(xnext[i] - x[i]);
    }
    return err;
}



double compute_deadend(graph *g, double *x, double d) {
    double st = 0.0;
    for (int i = 0; i < g->N; i++) {
        if (g->out[i] == 0) {
            st += x[i];
        }
    }
    return (d/g->N)*st;
}



double aux(graph *g, double *x, inmap *node) {
    if (node == NULL) {
        return 0;
    } else if (g->out[node->val] > 0) {
        return x[node->val]/g->out[node->val] + aux(g, x, node->left) + aux(g, x, node->right);
    }
}


double compute_y(graph *g, double *x, int j, double d) {
    double y = 0;
    inmap *node = g->in[j];
    y += aux(g, x, node);
    return y;
}




void *compute_pagerank(void *arg) {
    compute_info *info = (compute_info *) arg;
    bool terminated = false;
    while (!terminated) {
        xpthread_mutex_lock(info->mutex, QUI);
        // while the computation has finished and the main has not started a new iteration, wait
        while ((*info->n_computed) == info->n) {
            xpthread_cond_wait(info->start_pagerank_computation, info->mutex, QUI);
        }
        // check if the x value contains -1.0: in that case, end the computation for this thread
        if (info->x[0] == -1.0) {
            xpthread_mutex_unlock(info->mutex, QUI);
            terminated = true;
            break;
        }
        
        
        
        // calculate the sum(Y_i) term for the current X(t+1) element (at position *info->position)
        double sum_y = compute_y(info->g, info->x, (*info->position), info->d);
        
        // set in the xnext[position] the calculated value
        info->xnext[(*info->position) % info->n] = info->teleport + info->d*sum_y;
        (*info->position)++;
        (*info->n_computed)++;

        // if the threads computed every element of X(t+1), signal to the main thread to start a new iteration
        if ((*info->n_computed) == info->n) {
            xpthread_cond_signal(info->end_pagerank_computation, QUI);
        }
        xpthread_mutex_unlock(info->mutex, QUI);
    }
    return 0;
}







double *pagerank(graph *g, double d, double eps, int maxiter, int taux, int *numiter) {
    // define the 3 used arrays: x, y and xnext
    double *x = malloc(sizeof(double) * g->N);
    double *y = malloc(sizeof(double) * g->N);
    double *xnext = malloc(sizeof(double) * g->N);

    // inizialization of the probability array X(1)
    for (int i = 0; i < g->N; i++) {
        x[i] = 1.0/g->N;
    }
    double teleport = (1.0 - d)/g->N; // teleporting term
    
    // initializing info to pass to the threads for the communication
    pthread_t *threads = malloc(sizeof(pthread_t) * taux);
    compute_info *infos = malloc(sizeof(compute_info) * taux);
    pthread_cond_t start_pagerank_computation = PTHREAD_COND_INITIALIZER;
    pthread_cond_t end_pagerank_computation = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    int n_computed = 0;
    int position = 0;
    pthread_cond_t start_error_computation = PTHREAD_COND_INITIALIZER;
    pthread_cond_t end_error_computation = PTHREAD_COND_INITIALIZER;
    pthread_cond_t start_y_computation = PTHREAD_COND_INITIALIZER;
    pthread_cond_t end_y_computation = PTHREAD_COND_INITIALIZER;
    pthread_cond_t start_deadend_computation = PTHREAD_COND_INITIALIZER;
    pthread_cond_t end_deadend_computation = PTHREAD_COND_INITIALIZER;

    // set up information for the auxiliary threads
    for (int i = 0; i < taux; i++) {
        infos[i].start_pagerank_computation = &start_pagerank_computation;
        infos[i].end_pagerank_computation = &end_pagerank_computation;
        infos[i].mutex = &mutex;
        infos[i].g = g;
        infos[i].n_computed = &n_computed;
        infos[i].position = &position;
        infos[i].teleport = teleport;
        infos[i].d = d;
        infos[i].x = x;
        infos[i].y = y;
        infos[i].xnext = xnext;
        infos[i].n = g->N;

        infos[i].start_error_computation = &start_error_computation;
        infos[i].end_error_computation = &end_error_computation;
        infos[i].start_y_computation = &start_y_computation;
        infos[i].end_y_computation = &end_y_computation;
        infos[i].start_deadend_computation = &start_deadend_computation;
        infos[i].end_deadend_computation = &end_deadend_computation;
        infos[i].start_index_x = (i/taux)*g->N;
        infos[i].end_index_x = ((i+1)/taux)*g->N;
        xpthread_create(&threads[i], NULL, &compute_pagerank, &infos[i], QUI);
    }

    bool terminated = false; // pagerank computation terminated?

    
    // create the SIGUSR1 manager thread
    signal_info *sig_info = malloc(sizeof(signal_info));
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1); // add SIGUSR1 to the list of blocked signals
    pthread_sigmask(SIG_BLOCK, &mask, NULL); // block SIGUSR1 for this thread (main thread)
    // create the signal manager thread
    pthread_t *sig_manager = malloc(sizeof(pthread_t));
    // pass the data to the signal manager
    sig_info->numiter = numiter;
    sig_info->x = x;
    sig_info->terminated = &terminated;
    sig_info->n = g->N;
    xpthread_create(sig_manager, NULL, &manage_signal, sig_info, QUI);



    while (!terminated) {
        xpthread_mutex_lock(&mutex, QUI);
        // wait until the number of computed elements of xnext is equal to the number of nodes (all pageranks are calculated for this iteration)
        while (n_computed < g->N) {
            xpthread_cond_wait(&end_pagerank_computation, &mutex, QUI);
        }
        
        
        
        
        double st = compute_deadend(g, x, d);
        
        
        
        
        for (int i = 0; i < g->N; i++)  {
            xnext[i] += st;
        }
        
        
        
        double error = compute_error(xnext, x, infos->n);
        
        
        
        for (int i = 0; i < g->N; i++)  {
            x[i] = xnext[i];
        }; // set the calculated array to the previous
        position = 0; // reset the position
        n_computed = 0; // reset the computed counter to 0
        // calculate error
        (*numiter)++;
        if (*numiter == maxiter || error <= eps) {
            // warn the consumer threads that the computation has come to an end by sending an array of -1 elements
            for (int i = 0; i < g->N; i++) {
                x[i] = -1.0;
            }
            terminated = true;
        }
        xpthread_cond_broadcast(&start_pagerank_computation, QUI);
        xpthread_mutex_unlock(&mutex, QUI);
    }

    // wait for all the threads to terminate execution
    for (int i = 0; i < taux; i++) {
        xpthread_join(threads[i], NULL, QUI);
    }
    // xpthread_join(*sig_manager, NULL, QUI);

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


