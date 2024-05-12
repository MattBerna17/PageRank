# PageRank
PageRank algorithm in C (University of Pisa, sophomore year, final project of Laboratory II class)

## Code
### `pagerank.c`
The main file is called `pagerank.c`, and contains the `main` function, which, first of all, checks the arguments with the `getopt()` function: the only mandatory argument is `infile`, which is the name for the [*.mtx*](https://math.nist.gov/MatrixMarket/formats.html#MMformat) file: it should contain the following data:
$$
r \: c \: n \newline
i_1 \: j_1 \newline
... \newline
i_n \: j_n
$$
with $ n $ the number of edges, and $r$ and $c$ the number of nodes $N$ of the adjacent matrix.

The other optional arguments are:
- `k`: the number of top nodes at the end of the computation,
- `m`: the maximum number of iterations of the algorithm (to assure the termination),
- `d`: the damping factor, the probability of following the correlated links of the current page, while $\frac{1-d}{N}$ is the probability to jump to another node (not linked in the current page).
- `e`: the maximum error.
