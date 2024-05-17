# PageRank
PageRank algorithm in C (University of Pisa, sophomore year, final project of Laboratory II class)

## Documentation
### UML Scheme
![uml scheme](./Other/uml%20scheme.png)
In the following, I'll explain each of these components (except from `xerrori.h`, which we already discussed in class).

### `helpers.h` and `helpers.c`
These files contain the signature and the implementation (respectively) for each *non-pagerank strongly related* function. By adding these 2 files, I wanted to create a stronger level of abstraction, allowing reusability for these files and keeping the core files (`pagerank.c`, `utilities_pagerank.c` and `utilities_pagerank.h`) clearer and more focused on the actual project.

The `helpers.h` file contains the signature and the documentation for each function.

The `helpers.c` file contains the implementation, with inline commenting for the following functions:
- `void printerr(char *msg, char *file, int line)`: prints the error message passed, indicating the file and line, and then exits the program,
- `int readline(char *line, FILE *f)`: reads the first line of the file passed and checks for its ending, then uses the `line` array of chars to store the read value. It uses `getfirstchar` to check for the line content: if it's `%`, returns 1, if the line contains 3 integers (`r`, `c`, `n`) returns 3 and if the line contains an edge (2 integers, `i`, `j`) returns 2.


### `utilities_pagerank.h` and `utilities_pagerank.c`
These files define and implement the core functionalities needed by the main program, while the `.h` file also provides the data structures used to pass and store data:

The `.h` file contains the following data structures:
- `struct inmap`: a tree representing the input nodes for each destination node.
- `struct graph`: the graph structure, containing the number of nodes `N`, the array containing the number of output edges for each node `out`, and the array of pointers to the tree representing the input nodes for each node.
- `structure edge`: the structure to send and recive edges read from the *.mtx* file.
- `structure input_info`: structure used to pass synchronization mechanisms and data from main thread to consumer threads during the *.mtx* file read.

- `bool add(inmap** t, int val)`: a recursive function which returns `true` if the value can be added in the tree (adding it), and `false` if the value is already present in the tree.
- `void clear(inmap *t)`: function to free the nodes of the `t` inmap tree.
- `void *manage_edges(void *arg)`: the function called by each thread during the file read phase. `arg` is soon casted to a `input_info` instance, containing all of the information used by the threads to communicate with each other. Each thread follows a **producer-consumer** approach, using condition variables and a mutex. If available, a consumer thread (executing this function) fetches an `edge` instance from the array and checks whether its content is `NULL` or an actual edge: in the first case, it breaks out of the `while` loop, otherwise, else, it takes the source and destination of the edge and updates the related fields in `g->out[edge->src]` and `g->in[edge->dest]` (calling the `add` function, explained earlier), then `free`s the memory location containing the edge.


### `pagerank.c`
The main file is called `pagerank.c`, and contains the `main` function, which, first of all, checks the arguments with the `getopt()` function: the only mandatory argument is `infile`, which is the name for the [*.mtx*](https://math.nist.gov/MatrixMarket/formats.html#MMformat) file: it should contain the following data:
$$\displaylines{r\ c\ n \\\ i_1 \ j_1 \\\ ... \\\ i_n \ j_n}$$
with $n$ the number of edges, and $r$ and $c$ the number of nodes $N$ of the adjacent matrix.

The other optional arguments are:
- `k`: the number of top nodes at the end of the computation,
- `m`: the maximum number of iterations of the algorithm (to assure the termination),
- `d`: the damping factor, the probability of following the correlated links of the current page, while $\frac{1-d}{N}$ is the probability to jump to another node (not linked in the current page).
- `e`: the maximum error.

Following up, the main thread executing the file also creates the threads used to manage the edges during the file read operation.

First of all, the main thread reads the first few lines of the *.mtx* file, ignoring comments and, once arrived to the $r\ c\ n$ line, initializes the graph instance, setting `g->N = r`, placing zeros in the `g->out` array and `NULL`s in the `g->in` array of `inmap`s.

Then, it reads each line with $i\ j$ and creates an instance of edge with these values, places this instance in the array and notifies the consumer threads. At the end of the file, the main thread creates `t` `NULL`-filled `edge` instances, places them in the array and then waits for the threads to finish with `join` calls.