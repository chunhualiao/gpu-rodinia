/*
BFS Algorithm is from the paper: 

P. Harish and P. Narayanan. Accelerating large graph algorithms on the GPU using CUDA. 
In Proceedings of 2007 International Conference on High Performance Computing, Dec 2007.

" We solve the BFS problem using level synchronization. BFS traverses the graph in levels; once a level is visited it is not visited again. 

The BFS frontier corresponds to all the nodes being processed at the current level. 
* We do not maintain a queue for each vertex during our BFS execution because it will incur additional overheads of maintaining new array indices and changing the grid configuration at every level of kernel execution. 
** This slows down the speed of execution on the CUDA model.
* For our implementation we give one thread to every vertex. 
** Two boolean arrays, frontier and visited, Fa and Xa respectively, of size jVj are created which store the BFS frontier and the visited vertices. 
** Another integer array, cost, Ca, stores the minimal number of edges of each vertex from the source vertex S. 

In each iteration, each vertex looks at its entry in the frontier array Fa. 
* If true, it fetches its cost from the cost array Ca and updates all the costs of its neighbors if more than its own cost plus one using
the edge list Ea. 
* The vertex removes its own entry from the frontier array Fa and adds to the visited array Xa. 
* It also adds its neighbors to the frontier array if the neighbor is not already visited. 

This process is repeated until the frontier is empty. This algorithm needs iterations of order of the diameter of the graph G(V;E) in the worst case.
"

input_file's content: store nodes, edges and costs
---------------------
100  // number of nodes
0 7  // each row is for one node, starting from node 0: two numbers: first one the start id of its first edge, second number:total edges of the node
7 3
10 7

...
559 8 
567 5  // last node's first edge id, and total edge count

24 // start node's id: source node id

572  // total number of edges: this should last node's first edge id+ edge count: 567+5=572
0 8 // each row now is for one edge, starting from edge id=0, first number: destination node id, second number: cost of the edge
0 8
20 10
..

17 2
73 5


Documented by C. Liao, 2020
*/
#include <cstdio>
#include <string>
#include <math.h>
#include <cstdlib>
#include <omp.h>
//#define NUM_THREAD 4
#define OPEN

using namespace std;

FILE *fp;

//Structure to hold a node information
struct Node
{
	int starting;
	int no_of_edges;
};

void BFSGraph(int argc, char** argv);

void Usage(int argc, char**argv){

fprintf(stderr,"Usage: %s <num_threads> <input_file>\n", argv[0]);

}
////////////////////////////////////////////////////////////////////////////////
// Main Program
////////////////////////////////////////////////////////////////////////////////
int main( int argc, char** argv) 
{
	BFSGraph( argc, argv);
}



////////////////////////////////////////////////////////////////////////////////
//Apply BFS on a Graph using CUDA
////////////////////////////////////////////////////////////////////////////////
void BFSGraph( int argc, char** argv) 
{
        int no_of_nodes = 0;
        int edge_list_size = 0;
        char *input_f;
	int	 num_omp_threads;
	
	if(argc!=3){
	Usage(argc, argv);
	exit(0);
	}
    
	num_omp_threads = atoi(argv[1]);
	input_f = argv[2];
	
	printf("Reading File\n");
	//Read in Graph from a file
	fp = fopen(input_f,"r");
	if(!fp)
	{
		printf("Error Reading graph file\n");
		return;
	}

	int source = 0;

	fscanf(fp,"%d",&no_of_nodes);
   
	int total_mem=0; 

	// allocate host memory
	Node* h_graph_nodes = (Node*) malloc(sizeof(Node)*no_of_nodes);
	total_mem += sizeof(Node)*no_of_nodes;

	// flags to indicate if a node is the current level of nodes in the BFS traversal
	bool *h_graph_mask = (bool*) malloc(sizeof(bool)*no_of_nodes);
	total_mem += sizeof(bool)*no_of_nodes;


	// flags to mark next level's nodes: children of current level's nodes
	bool *h_updating_graph_mask = (bool*) malloc(sizeof(bool)*no_of_nodes);
	total_mem += sizeof(bool)*no_of_nodes;

	// if a node is visited or not
	bool *h_graph_visited = (bool*) malloc(sizeof(bool)*no_of_nodes);
	total_mem += sizeof(bool)*no_of_nodes;

	int start, edgeno;   
	// initalize the memory
	for( unsigned int i = 0; i < no_of_nodes; i++) 
	{
		fscanf(fp,"%d %d",&start,&edgeno);
		h_graph_nodes[i].starting = start;
		h_graph_nodes[i].no_of_edges = edgeno;
		h_graph_mask[i]=false;
		h_updating_graph_mask[i]=false;
		h_graph_visited[i]=false;
	}

	//read the source node from the file
	fscanf(fp,"%d",&source);
	// source=0; //tesing code line

	//set the source node as true in the mask
	h_graph_mask[source]=true;
	h_graph_visited[source]=true;

	fscanf(fp,"%d",&edge_list_size);

	int id,cost;
	int* h_graph_edges = (int*) malloc(sizeof(int)*edge_list_size);
	total_mem += sizeof(int)*edge_list_size;
	for(int i=0; i < edge_list_size ; i++)
	{
		fscanf(fp,"%d",&id);
		fscanf(fp,"%d",&cost);
		h_graph_edges[i] = id;
	}

	if(fp)
		fclose(fp);    


	// allocate mem for the result on host side
	int* h_cost = (int*) malloc( sizeof(int)*no_of_nodes);
	total_mem += sizeof(int)*no_of_nodes;

	for(int i=0;i<no_of_nodes;i++)
		h_cost[i]=-1;
	h_cost[source]=0;
	
	printf("Start traversing the tree using %d threads\n", num_omp_threads);
	printf ("Node count=%d, Edge count=%d, Memory Footprint=%d k bytes\n", no_of_nodes, edge_list_size, total_mem/1024);
	
	int k=0;
#ifdef OPEN
        double start_time = omp_get_wtime();
        omp_set_num_threads(num_omp_threads);
#ifdef OMP_OFFLOAD
#pragma omp target data map(to: no_of_nodes, h_graph_mask[0:no_of_nodes], h_graph_nodes[0:no_of_nodes], h_graph_edges[0:edge_list_size], h_graph_visited[0:no_of_nodes], h_updating_graph_mask[0:no_of_nodes]) map(h_cost[0:no_of_nodes])
        {
#endif 
#endif
// This is a confusing variable name: 
// It really means if there are nodes at the next level of graph in the context level-by-level traversal of BFS 
	bool stop; 
	do
        {
            //if no thread changes this value then the loop stops
            stop=false;

#ifdef OPEN
            //omp_set_num_threads(num_omp_threads);
    #ifdef OMP_OFFLOAD
    #pragma omp target teams distributed parallel for
    #else	    
    #pragma omp parallel for 
    #endif
#endif 
            for(int tid = 0; tid < no_of_nodes; tid++ )
            {
                if (h_graph_mask[tid] == true){ 
                    h_graph_mask[tid]=false;
                    for(int i=h_graph_nodes[tid].starting; i<(h_graph_nodes[tid].no_of_edges + h_graph_nodes[tid].starting); i++)
                    {
                        int id = h_graph_edges[i];
                        if(!h_graph_visited[id])
                        {
                            h_cost[id]=h_cost[tid]+1;
                            h_updating_graph_mask[id]=true;  // mark the neighbor nodes as candidates of next level of traversal
                        }
                    }
                }
            }

#ifdef OPEN
    #ifdef OMP_OFFLOAD
    #pragma omp target teams distributed parallel for map(stop)
    #else	    
    #pragma omp parallel for
    #endif
#endif
            for(int tid=0; tid< no_of_nodes ; tid++ )  // transfer the value of h_updating_graph_mask to h_graph_mask, also update h_graph_visited
            {
                if (h_updating_graph_mask[tid] == true){
                    h_graph_mask[tid]=true;
                    h_graph_visited[tid]=true; // this can be moved into the previous loop.
                    stop=true;
                    h_updating_graph_mask[tid]=false;
                }
            }
            k++;
        }
	while(stop);
#ifdef OPEN
        double end_time = omp_get_wtime();
        printf("Compute time: %lf seconds \n", (end_time - start_time));
#ifdef OMP_OFFLOAD
        }
#endif
#endif
	string output_file="result_";
	output_file+=to_string(no_of_nodes)+".txt";
	//Store the result into a file
	// The cost is the number of edges from source node to other nodes: the height of the BFS.
	FILE *fpo = fopen(output_file.c_str(),"w");
	for(int i=0;i<no_of_nodes;i++)
		fprintf(fpo,"%d) cost:%d\n",i,h_cost[i]);
	fclose(fpo);
	printf("Result stored in %s\n", output_file.c_str());


	// cleanup memory
	free( h_graph_nodes);
	free( h_graph_edges);
	free( h_graph_mask);
	free( h_updating_graph_mask);
	free( h_graph_visited);
	free( h_cost);

}

