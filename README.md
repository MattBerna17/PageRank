# PageRank
PageRank algorithm in C (University of Pisa, sophomore year, final project of Laboratory II class)

## Important Information
To develop this project, I made some choices, which I'll explain in the following:
- `inmap` implementation: I used a linked list, where each node has an int value (the index of the source node of the edge) and a pointer to the next node. The linked list is kept in descending order during the creation of the graph. To add the nodes in the list, I kept a list of mutex: to modify the `inmap` of the node destination $i$, I lock the $i-th$ mutex, and release it as soon as the add finishes.
- Also, to modify the `out` vector of the graph, I lock the graph mutex, added in the `grafo` struct.
- In the initial phase (reading the *.mtx* file), the main thread sequentially reads each line of the file, creates the edge described by the line, and sends it to the consumer threads, whom manage each edge checking their validity (if the edge is legal, i.e. the source and destination nodes are between 0 and N-1 number of nodes), and eventually adding them in the graph via the `inmap` struct and the `out` vector.
- To compute the *pagerank*, I divided the computation of each iteration in 4 steps, executed concurrently and dinamically (the first free thread computes the next available item, not using the static division of the arrays), divided by 4 barriers:
    1. Compute the Y vector
    2. Compute the St value
    3. Compute the X(t+1) vector
    4. Compute the error
    whenever each phase is completed, the consumer threads wait for the main to complete its operations, and then it signals the start of the next phase using the `signal` on the barrier.
- To kill the signal manager, the main thread sends a `SIGUSR2` signal to the signal manager, whom checks for the end of the pagerank computation and, in that case, exits.
