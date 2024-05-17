#include "utilities_pagerank.h"
#define HERE __FILE__, __LINE__
#define QUI __LINE__, __FILE__

void print(inmap* t) {
    if (t == NULL) {
        printf("Print null\n");
        return;
    }
    printf("Node: %d\n", t->val);
    if (t->left != NULL) {
        printf("\tLeft: %d\n", t->left->val);
    }
    if (t->right != NULL) {
        printf("\tRight: %d\n", t->right->val);
    }
    print(t->left);
    print(t->right);
}



// bool add(inmap **t, int val) {
//     inmap *node = malloc(sizeof(inmap));
//     node->val = val;
//     node->left = NULL;
//     node->right = NULL;

//     if (*t == NULL) {
//         // if the tree is empty, place the node and return true
//         printf("NULL: %d\n", val);
//         *t = node;
//         return true;
//     }
//     printf("TREE COMPARE: %d, %d\n", val, (*t)->val);
//     if (val == (*t)->val) {
//         // if the value is already in the tree, return false
//         free(node);
//         return false;
//     } else if (val < (*t)->val) {
//         // if val is less (go to the left)
//         if ((*t)->left == NULL) {
//             // if node is null, place it
//             (*t)->left = node;
//             return true;
//         } else {
//             // else, recursive search
//             free(node);
//             add(&(*t)->left, val);
//         }
//     } else if (val > (*t)->val) {
//         if ((*t)->right == NULL) {
//             (*t)->right = node;
//             return true;
//         } else {
//             free(node);
//             add(&(*t)->right, val);
//         }
//     }
// }


// bool add(inmap **t, int val) {
//     if (*t == NULL) {
//         // Crea un nuovo nodo se l'albero è vuoto o si è arrivati a una foglia
//         *t = (inmap *)malloc(sizeof(inmap));
//         if (*t == NULL) {
//             perror("Errore di allocazione della memoria");
//             exit(EXIT_FAILURE);
//         }
//         (*t)->val = val;
//         (*t)->left = NULL;
//         (*t)->right = NULL;
//         return true;
//     } else if (val < (*t)->val) {
//         // Ricorsivamente aggiungi il valore al sottoalbero sinistro
//         return add(&((*t)->left), val);
//     } else if (val > (*t)->val) {
//         // Ricorsivamente aggiungi il valore al sottoalbero destro
//         return add(&((*t)->right), val);
//     } else {
//         // Il valore è già presente nell'albero
//         return false;
//     }
// }


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
        while(*(info->available) == 0) {
            xpthread_cond_wait(info->canread, info->mutex, QUI);
        }
        // at least 1 available edge in arr
        edge *curr_edge = info->arr[(*info->position) % info->n];
        (*info->available)--;
        (*info->position)++;
        // check on the edge
        if (curr_edge == NULL) {
            // reached end of file, terminate thread execution
            xpthread_cond_signal(info->canwrite, QUI);
            xpthread_mutex_unlock(info->mutex, QUI);
            break;
        } else {
            // if the edge is from a node to itself, continue
            if (curr_edge->src != curr_edge->dest) {
                // else, add the node in the inmap structure
                // bool added = add(info->g->in[curr_edge->dest], curr_edge->src);
                // if succeded to add, increment number of out edges from src
                // printf("edge (%d, %d): %d\n", curr_edge->src, curr_edge->dest, added);
                bool added = true;
                if (added) {
                    info->g->out[curr_edge->src]++;
                }
            }
            xpthread_cond_signal(info->canwrite, QUI);
            xpthread_mutex_unlock(info->mutex, QUI);
        }
    }
    pthread_exit(NULL);
}


double *pagerank(graph *g, double d, double eps, int maxiter, int *numiter) {
    return 0;
}



