// used for random graph generation
#include <iostream>
#include <vector>
#include <random>
#include <cstdlib>
#include <ctime>
#include <climits>

// These names may vary by implementation
#if __GNUC__ 
#define LINEAR_CONGRUENTIAL_ENGINE linear_congruential_engine
#define UNIFORM_INT_DISTRIBUTION uniform_int_distribution
#else
#define LINEAR_CONGRUENTIAL_ENGINE linear_congruential
#define UNIFORM_INT_DISTRIBUTION uniform_int
#endif

using namespace std;

#define MIN_NODES 20
#define MAX_NODES ULONG_MAX
#define MIN_EDGES 2
#define MAX_INIT_EDGES 4 // Nodes will have, on average, 2*MAX_INIT_EDGES edges
#define MIN_WEIGHT 1
#define MAX_WEIGHT 10

typedef unsigned int uint;
typedef unsigned long ulong;

// interal graph's edge
struct edge {
  ulong dest; // destination node's ID
  uint weight;
};

//Structure to hold a node information
struct Node
{
        int starting;
        int no_of_edges;
};

//------------end for random graph generation

void generate_random_graph (int no_of_nodes, int & edge_list_size, Node*& h_graph_nodes, int*& h_graph_edges, int& source, int & total_mem)
{
  printf("Generating a random undirected graph with %d nodes\n", no_of_nodes);
  if ( no_of_nodes < MIN_NODES || no_of_nodes > MAX_NODES)
  { 
    std::cerr << "Error: Invalid node count argument from argv[3]: " << no_of_nodes << "\n";
    exit( 1 );
  }

  // Step 1: First generate a temporary graph

  // Adjacent list graph representation: each node has a list of linked nodes through edges
  typedef vector<edge> node;
  node * graph;
  graph = new node[no_of_nodes];
  // Initialize random number generators
  // C RNG for numbers of edges and weights
  srand( time( NULL ) );
  // TR1 RNG for choosing edge destinations
  LINEAR_CONGRUENTIAL_ENGINE<ulong, 48271, 0, ULONG_MAX> gen( time( NULL ) );
  UNIFORM_INT_DISTRIBUTION<ulong> randNode( 0, no_of_nodes - 1 );

  // Generate graph
  uint numEdges;
  ulong nodeID;
  uint weight;
  ulong i;
  uint j;
  for ( i = 0; i < no_of_nodes ; i++ )
  {
    numEdges = rand() % ( MAX_INIT_EDGES - MIN_EDGES + 1 ) + MIN_EDGES;
    for ( j = 0; j < numEdges; j++ )
    { 
      nodeID = randNode( gen );
      weight = rand() % ( MAX_WEIGHT - MIN_WEIGHT + 1 ) + MIN_WEIGHT;
      // build current_node --> dest_node
      graph[i].push_back( edge() );
      graph[i].back().dest = nodeID;
      graph[i].back().weight = weight;

      // Add dest_node --> current_node 
      graph[nodeID].push_back( edge() );
      graph[nodeID].back().dest = i;
      graph[nodeID].back().weight = weight;
    }
  }

  //---------------
  // Step 2: convert the temp graph into two lists : nodes and edges
  //
  // allocate host memory
  h_graph_nodes = (Node*) malloc(sizeof(Node)*no_of_nodes);
  total_mem += sizeof(Node)*no_of_nodes;

  ulong totalEdges = 0;
  // initalize the memory
  for( unsigned int i = 0; i < no_of_nodes; i++)
  {
    int numEdges = graph[i].size();
    h_graph_nodes[i].starting = totalEdges;
    h_graph_nodes[i].no_of_edges = numEdges;
    totalEdges += numEdges;
  }

  // the source node
  source = randNode( gen );

  edge_list_size= totalEdges;

  h_graph_edges = (int*) malloc(sizeof(int)*edge_list_size);
  total_mem += sizeof(int)*edge_list_size;
  int edge_id=0;
  for ( ulong i = 0; i < no_of_nodes; i++ )
    for ( uint j = 0; j < graph[i].size(); j++ )
    { 
      h_graph_edges[edge_id++] = graph[i][j].dest;
      // cost is not used
    }

  delete[] graph; // release the temporary graph.
}

