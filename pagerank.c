#include "utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    // read options from command line
    int opt;
    int k = 3, m = 100;
    float d = 0.9;
    long e = 10000000;
    char *infile;

    // : --> needs an argument.
    while((opt = getopt(argc, argv, "k:m:d:e:h")) != -1) {
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
            // to print help message
            case 'h':
                fprintf(stderr,"PageRank algorithm calculator.\nUsage: %s [-k K] [-m M] [-d D] [-e E] infile\n\nPositional arguments:\n \tinfile\t\tInput file in .mtx format (see https://math.nist.gov/MatrixMarket/formats.html#MMformat)\n\nOptions:\n\t-k\t\tShow the top K nodes (default: 3)\n\t-m\t\tMaximum number of operations (default: 100)\n\t-d\t\tDamping factor (default: 0.9)\n\t-e\t\tMax error (default: 1.0e7)\n", argv[0]);
                exit(0);
            case '?':
                fprintf(stderr, "[ERROR]: parameter unrecognized. Terminating.");
                exit(1);
            default:
                fprintf(stderr, "Usage: %s [-k K] [-m M] [-d D] [-e E] infile", argv[0]);
        }
    }

    // to check if there is the infile positional argument
    if (optind >= argc) {
        fprintf(stderr, "[ERROR]: Expected argument after options\n");
        exit(1);
    }
    infile = argv[optind];

    hello();
}