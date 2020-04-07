#ifndef GTREE_H
#define GTREE_H

#include<stdio.h>
#include<metis.h>
#include<vector>
#include<stdlib.h>
#include<memory.h>
#include<unordered_map>
#include<map>
#include<set>
#include<deque>
#include<stack>
#include<algorithm>
#include<sys/time.h>
#include<iostream>
using namespace std;
// MACRO for timing
extern struct timeval tv;
extern long long ts, te;
#define TIME_TICK_START gettimeofday( &tv, NULL ); ts = tv.tv_sec * 100000 + tv.tv_usec / 10;
#define TIME_TICK_END gettimeofday( &tv, NULL ); te = tv.tv_sec * 100000 + tv.tv_usec / 10;
#define TIME_TICK_PRINT(T) printf("%s RESULT: %lld (0.01MS)\r\n", (#T), te - ts );
// ----------

#define FILE_NODE "cal.cnode"
#define FILE_EDGE "cal.cedge"
// set all edge weight to 1(unweighted graph)
#define ADJWEIGHT_SET_TO_ALL_ONE true
// we assume edge weight is integer, thus (input edge) * WEIGHT_INFLATE_FACTOR = (our edge weight)
#define WEIGHT_INFLATE_FACTOR 100000
// gtree fanout
#define PARTITION_PART 2
// gtree leaf node capacity = tau(in paper)
#define LEAF_CAP 4
// gtree index disk storage
#define FILE_NODES_GTREE_PATH "cal.paths"
#define FILE_GTREE 			  "cal.gtree"
#define FILE_ONTREE_MIND	  "cal.minds"

#define FILE_OBJECT "cal.object"

typedef struct{
    double x,y;
    vector<int> adjnodes;
    vector<int> adjweight;
    bool isborder;
    vector<int> gtreepath; // this is used to do sub-graph locating
}Node;

typedef struct{
    vector<int> borders;
    vector<int> children;
    bool isleaf;
    vector<int> leafnodes;
    int father;
// ----- min dis -----
    vector<int> union_borders; // for non leaf node
    vector<int> mind; // min dis, row by row of union_borders
    vector< vector<int > > paths;
    //unordered_map<int, int> paths;
// ----- for pre query init, OCCURENCE LIST in paper -----
    vector<int> nonleafinvlist;
    vector<int> leafinvlist;
    vector<int> up_pos;
    vector<int> current_pos;
}TreeNode;

extern int noe; // number of edges
extern vector<Node> Nodes;
extern vector<TreeNode> GTree;

// use for metis
// idx_t = int64_t / real_t = double
extern idx_t nvtxs; // |vertices|
extern idx_t ncon; // number of weight per vertex
extern idx_t* xadj; // array of adjacency of indices
extern idx_t* adjncy; // array of adjacency nodes
extern idx_t* vwgt; // array of weight of nodes
extern idx_t* adjwgt; // array of weight of edges in adjncy
extern idx_t nparts; // number of parts to partition
extern idx_t objval; // edge cut for partitioning solution
extern idx_t* part; // array of partition vector
extern idx_t options[METIS_NOPTIONS]; // option array

extern void options_setting();
#endif // GTREE_H
