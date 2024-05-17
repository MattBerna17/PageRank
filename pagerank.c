#include "utilities_pagerank.h"
#define HERE __FILE__, __LINE__
#define QUI __LINE__, __FILE__

int main(int argc, char* argv[]) {
    // read options from command line
    int opt;
    int k = 3, m = 100, t = 3;
    float d = 0.9;
    double e = 1.e7;
    char *infile;

    // : --> needs an argument.
    while((opt = getopt(argc, argv, "k:m:d:e:t:h")) != -1) {
        switch (opt) {
            case 'k':
                k = atoi(optarg);
                break;
            case 'm':
                m = atoi(optarg);
                break;
            case 'd':
                d = atof(optarg);
                break;
            case 'e':
                e = atof(optarg);
                break;
            case 't':
                t = atoi(optarg);
                break;
            // to print help message
            case 'h':
                fprintf(stderr,"PageRank algorithm calculator.\nUsage: %s [-k K] [-m M] [-d D] [-e E] infile\n\nPositional arguments:\n \tinfile\t\tInput file in .mtx format (see https://math.nist.gov/MatrixMarket/formats.html#MMformat)\n\nOptions:\n\t-k\t\tShow the top K nodes (default: 3)\n\t-m\t\tMaximum number of operations (default: 100)\n\t-d\t\tDamping factor (default: 0.9)\n\t-e\t\tMax error (default: 1.0e7)\n\t-t\t\tNumber of auxiliary threads (default: 3)\n", argv[0]);
                exit(0);
            case '?':
                printerr("[ERROR]: Parameter unrecognized. Terminating.", HERE);
            default:
                fprintf(stderr, "Usage: %s [-k K] [-m M] [-d D] [-e E] [-t T] infile", argv[0]);
                exit(1);
        }
    }

    // to check if there is the infile positional argument
    if (optind >= argc) {
        printerr("[ERROR]: Expected argument after options. Terminating.", HERE);
    }
    infile = argv[optind];

    FILE *in = fopen(infile, "rt");
    if (in == NULL) {
        printerr("[ERROR]: Bad file name. Terminating.", HERE);
    }


    // create thread, read the file and communicate the lines read
    pthread_cond_t canread = PTHREAD_COND_INITIALIZER;
    pthread_cond_t canwrite = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    graph *g = malloc(sizeof(graph));
    int n = Buf_size;
    edge **arr = malloc(sizeof(edge*)*n);
    int available = 0;
    int cons_position = 0;
    int prod_position = 0;

    pthread_t threads[t];
    input_info infos[t];

    for (int i = 0; i < t; i++) {
        infos[i].canread = &canread;
        infos[i].canwrite = &canwrite;
        infos[i].mutex = &mutex;
        infos[i].g = g;
        infos[i].arr = arr;
        infos[i].n = n;
        infos[i].available = &available;
        infos[i].position = &cons_position;
        
        xpthread_create(&threads[i], NULL, &manage_edges, &infos[i], QUI);
    }


    size_t length = 0;
    char *line = NULL;
    int n_edges = 0;
    bool read = false;
    while (!read) {
        int e = read_line(&line, &length, in);
        // e contains the return code, line contains the actual file line read
        if (e == 1) {
            // if the line read is a comment, continue
            if (line[0] == '%') continue;
            else {
                // in line r c n
                int r, c;
                char *v = strtok(line, " ");
                r = atoi(v);
                v = strtok(NULL, " ");
                c = atoi(v);
                v = strtok(NULL, " ");
                n_edges = atoi(v);

                g->N = r; // number of nodes
                int *out = malloc(sizeof(int)*g->N); // define the array containing the number of out edges of each node
                inmap **in = malloc(sizeof(inmap*)*g->N);
                for (int i = 0; i < g->N; i++) {
                    out[i] = 0;
                    in[i] = NULL;
                }
                g->out = out;
                g->in = in;
                read = true;
            }
        } else {
            printerr("[ERROR]: Formatting error in the input file. Terminating.", HERE);
        }
    }


    int terminated_threads = 0;
    while(terminated_threads < t) {
        edge *curr_edge = malloc(sizeof(edge));
        int e = read_line(&line, &length, in);
        if (e == 1) {
            // the line contains i j, the edge from i to j
            // tokenize the string
            char *v = strtok(line, " ");
            int i = atoi(v);
            v = strtok(NULL, " ");
            int j = atoi(v);
            curr_edge->src = i - 1;
            curr_edge->dest = j - 1;
        } else if (e == 0) {
            // end of file, notify threads
            curr_edge = NULL;
            terminated_threads++;
        } else {
            printerr("[ERROR]: Error during file read. Terminating.", HERE);
        }
        // get the mutex to write on the buffer
        xpthread_mutex_lock(&mutex, QUI);
        // if the buffer is full, wait until at least one element is free
        while (available == n) {
            xpthread_cond_wait(&canwrite, &mutex, QUI);
        }
        // add the edge in the buffer
        arr[prod_position%n] = curr_edge;
        prod_position++;
        available++;
        xpthread_cond_signal(&canread, QUI); // notify the consumers that an edge is available
        xpthread_mutex_unlock(&mutex, QUI);
    }

    fprintf(stdout, "Number of nodes: %d\n", g->N);
    int count_edges = 0;
    int count_dead_ends = 0;
    for (int i = 0; i < g->N; i++) {
        if (g->out[i] == 0) {
            count_dead_ends++;
        } else {
            count_edges += g->out[i];
        }
    }

    fprintf(stdout, "Number of dead-end nodes: %d\n", count_dead_ends);
    fprintf(stdout, "Number of valid arcs: %d\n", count_edges);
    xpthread_mutex_destroy(&mutex, QUI);

    return 0;
}


