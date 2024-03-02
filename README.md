# hw1

# PageRank Algorithm
This program implements the PageRank algorithm using parallel processing with pthreads. The PageRank algorithm is used 
by search engines to rank web pages in their search results. As a metric it is using the number of references
form other important pages that each page has. 

## Configuration/Constants
- `NUM_OF_THREADS`: Number of threads to be used for parallel processing.(Adjust the NUM_OF_THREADS constant according 
to the available hardware resources for optimal performance.)
- `DAMPING_FACTOR`: Damping factor used in the PageRank algorithm (usually set to 0.85).
- `REPETITIONS_OF_PAGERANG`: Number of iterations for PageRank computation(more iteration = more accurate).
- `OUTPUT_FILE_NAME`: Name of the output file to store the PageRank results.

## Dependencies
- `stdio.h`: Standard Input and Output library.
- `stdlib.h`: Standard library providing memory allocation functions.
- `string.h`: String manipulation functions.
- `time.h`: Time functions.
- `pthread.h`: POSIX Threads library for parallel processing.

## Execution
1. In the same directory as the Makefile run make
2. Run the executable with one comantline argument the path of the datafile(./ask1 pathOfDataFile).
4. The program will compute the PageRank algorithm using the specified number of threads and store the results in the output file.

## Structures
- `graph_node_t`: Structure representing a node in the graph.
    - `num`: Node number.
    - `rank`: PageRank value of the node.
    - `incomingList`: Array of pointers to the nodes that link to this node.
    - `incomingCount`: Count of incoming edges.
    - `outgoingCount`: Count of outgoing edges. (used by the node it its connected to to divide the rank)


- `thread_params_PakeRank_t`: Structure representing parameters for parallel PageRank computation.
    - `firstNode`: First node index assigned to the thread.
    - `lastNode`: Last node index assigned to the thread.
    - `thread`: Thread identifier.

## Functions
- `createGraphNode`: Creates and inits a new graph node with the specified number.
- `addToIncomingList`: Adds a node to the incoming list of another node.
- `readData`: Reads data from a file and constructs the graph.
- `freeGraph`: Frees the allocated memory for the graph.
- `writePageRankToFile`: Writes the PageRank results to a CSV file.
- `printGraph`: Prints the graph representation.
- `parallelPageRank`: Function executed by each thread to compute PageRank in parallel.
- `main`: Entry point of the program.

## Implementation
- the readData function takes the file and constructs the graph with the data described above. 
the graph is a dynamic array and the num of the node is used as the intex so we have O(1) complexity
on the look ups each time we have to add a note that is bigger than the graph capacity we remalloc the array
with 2 times the size so we achieve the fewer traps to the operating system and having a complexity of O(n)
for the array resizing. Each time we a node we have not yet include (i can be eather in the scr or the dest
as we still want to have the nodes that only non other nodes are pointing to or the ones that dont point to any other node)
and we increment the praghsize.
and add it to the node. also for each link of src and dest we have to add the src to the incomingList of the dest,
increment the incomingCount of the dest as we just added one, and also increment the outgoingCount of the src as we just 
recorded an outgoing edge of his.
- after creating the gragh we have to devide the work between the threads. We follow the forall divide where
we must loop through a array and perform a task so each thread is assined a subarray to perform the task to.
to do that we use the thread_params_PakeRank_t struct and we pass is as the void * parameter of the function 
we call the thread on
- now in the parallelPageRank we try to limit as much as possible the write instructions in the nodes so we can avoid
the need for mutexes. Firstly each thread for each node its responsible for runs through the list of the incoming ndoes,
and using the rank and the number of the outcoming edges that these nodes have it can calculate the rank that he 'takes'
each node and the sum of all of those values is's holle incoming rank. Now that sum is saved in an array unique for each
thread. then we have to change the rank in the actual graph, but before we do that we have to make sure that all threads
have finished calculating the incoming ranks so we dont have any race conditions or a thread uses the next rank of a 
incoming node due to it being updated first. So all the thread wait on a barier equal to the number of threads.
Now all the threads have finished the calcualtion of the incoming rank so each thread can update the according nodes in
the graph graph[i]->rank = 1 - DAMPING_FACTOR + (DAMPING_FACTOR * the incomingRankSum of i);.
now the thread but againg rach a second barier so no thread starts the next repetition before all the threads have finished
updating the ranks. This prosess is repeted REPETITIONS_OF_PAGERANG times and then the graph has the final ranks.
- lastly we make the file with the results pulling the data from the graph

## Performance

to estemate the perfomance and the speed up the the parrarel function we will use three sampledata files:
1. p2p-Gnutella24: http://snap.stanford.edu/data/p2p-Gnutella24.txt.gz
2. ego-Facebook: http://snap.stanford.edu/data/facebook_combined.txt.gz
3. email-Enron: http://snap.stanford.edu/data/email-Enron.txt.gz

for each one we gonna calculate the performance by measuring the speed up of using 2,3 and 4 threads compaired to 1 thread.
we run each configuration 5 times and use the average of those
REPETITIONS_OF_PAGERANG = 50

1. p2p-Gnutella24: http://snap.stanford.edu/data/p2p-Gnutella24.txt.gz

with 1 thread: (0.098371, 0.097044, 0.099120, 0.097994, 0.097487), average = 0.0980032, standard deviation = 0.00071681668507366

with 2 thread: (0.075530, 0.053289, 0.053912, 0.050605, 0.051347), average = 0.0569366, standard deviation = 0.0093755053325141, speedup = 1,72126892

with 3 thread: (0.066144, 0.059100, 0.061307, 0.066890, 0.052457), average = 0.0611796, standard deviation = 0.0052474499178172, speedup = 1,601893442

with 4 thread: (0.041552, 0.067147, 0.047471, 0.049346, 0.052223), average = 0.0515478, standard deviation = 0.0085460537653352, speedup = 1,901210139


2. ego-Facebook: http://snap.stanford.edu/data/facebook_combined.txt.gz

with 1 thread: (0.068512, 0.066345, 0.066128, 0.067379, 0.067070), average = 0.0670868, standard deviation = 0.0008469265375462

with 2 thread: (0.061384, 0.061558, 0.061402, 0.061337, 0.055097), average = 0.0601556 , standard deviation = 0.0025303891084179, speedup = 1,115221193

with 3 thread: (0.050771, 0.057499, 0.029466, 0.057704, 0.026281), average = 0.0443442 , standard deviation = 0.013714786230926, speedup = 1,512865268

with 4 thread: (0.033000, 0.035996, 0.041433, 0.039252, 0.035589), average = 0.037054, standard deviation = 0.0029565977068245, speedup = 1,810514384 


3. email-Enron: http://snap.stanford.edu/data/email-Enron.txt.gz

with 1 thread: (0.297635, 0.293260, 0.294196, 0.295809, 0.264068), average = 0.2889936, standard deviation = 0.012551303846215

with 2 thread: (0.236384, 0.234892, 0.237687, 0.233574, 0.235901), average = 0.2356876, standard deviation = 0.0013876885241292, speedup = 1,226172272

with 3 thread: (0.211928, 0.212619, 0.214591, 0.218788, 0.218240), average =  0.2152332, standard deviation = 0.0028230571655565, speedup = 1,342699918

with 4 thread: (0.192969, 0.191741, 0.159414, 0.197865, 0.192200), average = 0.1868378, standard deviation = 0.013885604911562, speedup = 1,546761951

