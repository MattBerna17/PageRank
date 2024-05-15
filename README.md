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
- `char getfirstchar(char *str)`: returns the first *non-space* character from the passed string (useful for the following function),
- `int readline(char *line, FILE *f)`: reads the first line of the file passed and checks for its ending, then uses the `line` array of chars to store the read value. It uses `getfirstchar` to check for the line content: if it's `%`, returns 1, if the line contains 3 integers (`r`, `c`, `n`) returns 3 and if the line contains an edge (2 integers, `i`, `j`) returns 2.


### `utilities_pagerank.h` and `utilities_pagerank.c`
<>talk about data structures.

These files define and implement the core functionalities needed by the main program, while the `.h` file also provides the data structures used to pass and store data:
- `void *manage_edges(void *arg)`: the function called by each thread during the file read phase. `arg` is soon casted to a `input_info` instance, containing all of the information used by the threads to communicate with each other. Each thread follows a **producer-consumer** approach, using condition variables and a mutex. <>continues.


### `pagerank.c`
The main file is called `pagerank.c`, and contains the `main` function, which, first of all, checks the arguments with the `getopt()` function: the only mandatory argument is `infile`, which is the name for the [*.mtx*](https://math.nist.gov/MatrixMarket/formats.html#MMformat) file: it should contain the following data:
$$\displaylines{r\ c\ n \\\ i_1 \ j_1 \\\ ... \\\ i_n \ j_n}$$
with $n$ the number of edges, and $r$ and $c$ the number of nodes $N$ of the adjacent matrix.

The other optional arguments are:
- `k`: the number of top nodes at the end of the computation,
- `m`: the maximum number of iterations of the algorithm (to assure the termination),
- `d`: the damping factor, the probability of following the correlated links of the current page, while $\frac{1-d}{N}$ is the probability to jump to another node (not linked in the current page).
- `e`: the maximum error.

Following up, the main thread executing the file also creates the threads used to manage the edges during the file read operation. <>continues.