#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>


#define NUM_OF_THREADS 4
#define DAMPING_FACTOR 0.85
#define REPETITIONS_OF_PAGERANG 50
#define OUTPUT_FILE_NAME "pagerank.csv"

typedef struct graph_node_t{
	long int num;
	long double rank;
	struct graph_node_t **incomingList;
	long int incomingCount;
	long int outgoingCount;
}graph_node_t;


typedef struct thread_params_PakeRank{
	long int firstNode;
	long int lastNode;
	pthread_t thread;
}thread_params_PakeRank_t;

graph_node_t **graph;
long int graphCapasity = 100;
long int graphsize = 0;

pthread_barrier_t calcIncomingRanksBarrirer;


graph_node_t* createGraphNode(long int num) {
    graph_node_t* newNode = (graph_node_t*)malloc(sizeof(graph_node_t));
    if (newNode == NULL) {
        printf("Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    newNode->num = num;
    newNode->rank = 1.0;
    newNode->incomingList = NULL;
    newNode->incomingCount = 0;
    newNode->outgoingCount = 0;
    return newNode;
}


void addToIncomingList(graph_node_t *node, graph_node_t *incomingNode){
	node->incomingList = realloc(node->incomingList, ((node->incomingCount) + 1) * sizeof(graph_node_t*));
	if(node->incomingList == NULL){
		fprintf(stderr, "Memory allocation failed in incomingList\n");
		exit(EXIT_FAILURE);
	}
	node->incomingList[node->incomingCount] = incomingNode;
	node->incomingCount++;
	
	//if a node is incoming to another it means that it hase a outcoing aedge
	incomingNode->outgoingCount++;

	return;
}


//return the capasity of the nodes
void readData(FILE *file){
	long int src, dest;
	graph = (graph_node_t **)malloc(graphCapasity * sizeof(graph_node_t*));
	
	if(graph == NULL){
		fprintf(stderr, "Memory allocation failed\n");
		return exit(-1);
	}

    while (fscanf(file, "%ld %ld", &src, &dest) != 2) {
        // Skip the line if it starts with #
        char c;
        while ((c = fgetc(file)) != EOF && c != '\n');
    }


    do {
        //prosses src, dest
		
		//check if we need to resize
		if (src >= graphCapasity || dest >= graphCapasity){
			int minNewSize = (src >= dest ? src : dest) + 1;

			//double the size
			do{
				graphCapasity *= 2;
			}while (!(graphCapasity > minNewSize));
			graph = (graph_node_t**)realloc(graph, graphCapasity * sizeof(graph_node_t*));
			if(graph == NULL){
				fprintf(stderr, "Memory allocation failed wile resizing\n");
				return exit(-1);
			}
			//initialize with null the new nodes
			for(int i = graphsize; i < graphCapasity; i++)graph[i] = NULL;
		}

		//add the nodes in the graph
		if(graph[src] == NULL){
			graph[src] = createGraphNode(src);
			graphsize++;
		}

		if(graph[dest] == NULL){
			graph[dest] = createGraphNode(dest);
			graphsize++;
		}

		//add the src to the list of incomming edges of the dest
		addToIncomingList(graph[dest], graph[src]);

    }while (fscanf(file, "%ld %ld", &src, &dest) == 2);


	return;
}


void freeGraph(void){
	for(int i = 0; i < graphCapasity; i++){
		if(graph[i] != NULL){
			if(graph[i]->incomingList != NULL)free(graph[i]->incomingList);
			free(graph[i]);
		}
	}
	free(graph);
}

void writePageRankToFile() {
    FILE *file = fopen(OUTPUT_FILE_NAME, "w");
    if (file == NULL) {
        perror("Error in opening file");
        exit(EXIT_FAILURE);
    }

    // Write the header
    fprintf(file, "node,pagerank\n");

    // Write node numbers and their PageRank values
    for (int i = 0; i <= graphCapasity; i++) {
    	if(graph[i] == NULL)continue;
        fprintf(file, "%ld,%Lf\n", graph[i]->num, graph[i]->rank);
    }

    fclose(file);
}


void printGraph() {
    for (int i = 0; i <= graphsize; i++) {
		if(graph[i] == NULL)continue;

        printf("Node %ld (Rank: %Lf) - Incoming Nodes: ", graph[i]->num, graph[i]->rank);
        for (int j = 0; j < graph[i]->incomingCount; j++) {
            printf("%ld ", graph[i]->incomingList[j]->num);
        }
        printf("\n");
    }
}


void *parallelPageRank(void *_threadArgs){
    thread_params_PakeRank_t *threadArgs = (thread_params_PakeRank_t *)_threadArgs;
    long double incomingRankSum[threadArgs->lastNode - threadArgs->firstNode];
    
    for(int repetitions = REPETITIONS_OF_PAGERANG; repetitions != 0; repetitions--){
	    //for each node we are responsible for
	    for(long int i = threadArgs->firstNode; i <= threadArgs->lastNode; i++){
			if(graph[i] == NULL) continue;

			//collect the pageRank of all the incoming nodes
			incomingRankSum[i - threadArgs->firstNode] = 0;
			for(long int j = 0; j < graph[i]->incomingCount; j++){
				incomingRankSum[i - threadArgs->firstNode] += (graph[i]->incomingList[j]->rank) / (graph[i]->incomingList[j]->outgoingCount);
			}
	    }
	    //wait for all the threads to calculate the incoming rank 
	    //of all the nodes that they are responsible for
	    pthread_barrier_wait(&calcIncomingRanksBarrirer);
		
		//now there is no race condition or depentances so each thread can calculate the new ranks of the nodes
		//it is responsible for
		for(long int i = threadArgs->firstNode; i <= threadArgs->lastNode; i++){
			if(graph[i] == NULL)continue;

			graph[i]->rank = 1 - DAMPING_FACTOR + (DAMPING_FACTOR * incomingRankSum[i - threadArgs->firstNode]);
		}

		//we wait for all the threads to do that so no thread continiues the next repetition with old data 
		//or we have race condition
	    pthread_barrier_wait(&calcIncomingRanksBarrirer);
	}

    return NULL;
}


int main(int argc, char **argv){
	//char dataPath[MAX_PATH_LEN];
	FILE *file;

	thread_params_PakeRank_t theadArgs[NUM_OF_THREADS];
	long int step;
	int start = 0;

	//clock_t start_time, end_time;
    struct timespec starttime, end;

	pthread_barrier_init(&calcIncomingRanksBarrirer, NULL, NUM_OF_THREADS);

    if(argc != 2){
        perror("Wrong call\ncall with one command line argument (the filepath of the datafile)\n");
        exit(EXIT_FAILURE);
    }

	file = fopen(argv[1],"r");
	if(file == NULL){
        perror("Coundn't open file\n");
        exit(EXIT_FAILURE);
    }
	
	readData(file);

	step = graphsize/NUM_OF_THREADS;
	for(int i = 0; i<NUM_OF_THREADS; i++){
		theadArgs[i].firstNode = start;
		theadArgs[i].lastNode = (i < NUM_OF_THREADS-1) ? (start + step -1) : graphsize;
		start += step;

        printf("thread:%d got from nodenum:%ld to nodenum:%ld\n", i, theadArgs[i].firstNode, theadArgs[i].lastNode);
	}

    clock_gettime(CLOCK_MONOTONIC, &starttime);
	for(int i = 0; i<NUM_OF_THREADS; i++){
		pthread_create(&theadArgs[i].thread, NULL, &parallelPageRank, &theadArgs[i]);
	}


	for(int i = 0; i < NUM_OF_THREADS; i++){
		pthread_join(theadArgs[i].thread, NULL);
	}


    clock_gettime(CLOCK_MONOTONIC, &end);
	printf("The pageRank algorithm took: %f second using %d threads.\n", (end.tv_sec - starttime.tv_sec) +
                                                                         (end.tv_nsec - starttime.tv_nsec) / 1e9 , NUM_OF_THREADS);

	writePageRankToFile();

	fclose(file);
	freeGraph();
}
