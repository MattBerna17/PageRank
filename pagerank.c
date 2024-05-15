#include "utilities_pagerank.h"
#define HERE __FILE__, __LINE__

int main(int argc, char* argv[]) {
    // read options from command line
    int opt;
    int k = 3, m = 100, t = 3;
    float d = 0.9;
    long e = 10000000;
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
                e = atol(optarg);
                break;
            case 't':
                t = atoi(optarg);
                break;
            // to print help message
            case 'h':
                fprintf(stderr,"PageRank algorithm calculator.\nUsage: %s [-k K] [-m M] [-d D] [-e E] infile\n\nPositional arguments:\n \tinfile\t\tInput file in .mtx format (see https://math.nist.gov/MatrixMarket/formats.html#MMformat)\n\nOptions:\n\t-k\t\tShow the top K nodes (default: 3)\n\t-m\t\tMaximum number of operations (default: 100)\n\t-d\t\tDamping factor (default: 0.9)\n\t-e\t\tMax error (default: 1.0e7)\n\t-t\t\tNumber of auxiliary threads (default: 3)\n", argv[0]);
                exit(0);
            case '?':
                fprintf(stderr, "[ERROR]: Parameter unrecognized. Terminating.");
                exit(1);
            default:
                fprintf(stderr, "Usage: %s [-k K] [-m M] [-d D] [-e E] [-t T] infile", argv[0]);
        }
    }

    // to check if there is the infile positional argument
    if (optind >= argc) {
        fprintf(stderr, "[ERROR]: Expected argument after options\n");
        exit(1);
    }
    infile = argv[optind];

    FILE *in = fopen(infile, "rt");
    int code;
    char *line;
    do {
        code = readline(line, in);
    } while (code != 0);


    // create thread, read the file and communicate the lines read
    pthread_cond_t canread = PTHREAD_COND_INITIALIZER;
    pthread_cond_t canwrite = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    edge *arr[Buf_size];
    int n;
    int available = 0;

    pthread_t threads[t];
    input_info infos[t];

    for (int i = 0; i < t; i++) {
        pthread_create(&threads[i], NULL, &manage_edges, &infos[i]);
    }

    printf("TERMINO\n");
}