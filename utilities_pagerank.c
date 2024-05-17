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
    pthread_exit(NULL);
}


double *pagerank(graph *g, double d, double eps, int maxiter, int *numiter) {
    return 0;
}



