#include "utilities_pagerank.h"
#define HERE __FILE__, __LINE__
#define QUI __LINE__, __FILE__

bool add(inmap** t, int val) {
    if (*t == NULL) {
        inmap *el = malloc(sizeof(inmap));
        el->val = val;
        el->left = NULL;
        el->right = NULL;
        *t = el;
        return true;
    } else if ((*t)->val == val) {
        return false;
    } else {
        if ((*t)->val > val) {
            return add(&(*t)->left, val);
        } else if ((*t)->val < val) {
            return add(&(*t)->right, val);
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
    } else {
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
            xpthread_cond_wait(info->threads_can_proceed, info->mutex, QUI);
        }
        // check if the x value contains -1.0: in that case, end the computation for this thread
        if (info->x[0] == -1.0) {
            xpthread_mutex_unlock(info->mutex, QUI);
            terminated = true;
            break;
        }
        double sum_y = compute_y(info->g, info->x, (*info->position), info->d);
        // set in the xnext[position] the calculated value
        info->xnext[(*info->position) % info->n] = info->teleport + info->d*sum_y;
        (*info->position)++;
        (*info->n_computed)++;

        // if the threads computed every element of X(t+1), signal to the main thread to start a new iteration
        if ((*info->n_computed) == info->n) {
            xpthread_cond_signal(info->main_can_proceed, QUI);
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
    pthread_cond_t main_can_proceed = PTHREAD_COND_INITIALIZER;
    pthread_cond_t threads_can_proceed = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    int n_computed = 0;
    int position = 0;
    bool start = false;

    // set up information for the auxiliary threads
    for (int i = 0; i < taux; i++) {
        infos[i].main_can_proceed = &main_can_proceed;
        infos[i].threads_can_proceed = &threads_can_proceed;
        infos[i].mutex = &mutex;
        infos[i].g = g;
        infos[i].n_computed = &n_computed;
        infos[i].position = &position;
        infos[i].teleport = teleport;
        infos[i].start = &start;
        infos[i].d = d;
        infos[i].x = x;
        infos[i].y = y;
        infos[i].xnext = xnext;
        infos[i].n = g->N;

        xpthread_create(&threads[i], NULL, &compute_pagerank, &infos[i], QUI);
    }

    bool terminated = false;
    while (!terminated) {
        xpthread_mutex_lock(&mutex, QUI);
        start = true;
        // wait until the number of computed elements of xnext is equal to the number of nodes (all pageranks are calculated for this iteration)
        while (n_computed < g->N) {
            xpthread_cond_wait(&main_can_proceed, &mutex, QUI);
        }
        start = false;
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
        xpthread_cond_broadcast(&threads_can_proceed, QUI);
        xpthread_mutex_unlock(&mutex, QUI);
    }

    // wait for all the threads to terminate execution
    for (int i = 0; i < taux; i++) {
        xpthread_join(threads[i], NULL, QUI);
    }
    // free used data structures
    free(threads);
    free(infos);
    free(x);
    free(y);

    return xnext;
}



int cmp_ranks(const void *a, const void *b) {
    rank **r1 = (rank **) a;
    rank **r2 = (rank **) b;
    if ((*r1)->val < (*r2)->val) return 1;
    else if ((*r1)->val > (*r2)->val) return -1;
    else return 0;
}





