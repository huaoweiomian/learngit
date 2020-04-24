#include "gtree.h"
struct timeval tv;
long long ts, te;
 int noe; // number of edges
 vector<Node> Nodes;
 vector<TreeNode> GTree;

// use for metis
// idx_t = int64_t / real_t = double
 idx_t nvtxs; // |vertices|
 idx_t ncon; // number of weight per vertex
 idx_t* xadj; // array of adjacency of indices
 idx_t* adjncy; // array of adjacency nodes
 idx_t* vwgt; // array of weight of nodes
 idx_t* adjwgt; // array of weight of edges in adjncy
 idx_t nparts; // number of parts to partition
 idx_t objval; // edge cut for partitioning solution
 idx_t* part; // array of partition vector
 idx_t options[METIS_NOPTIONS]; // option array
 void options_setting(){
     METIS_SetDefaultOptions(options);
     options[METIS_OPTION_PTYPE] = METIS_PTYPE_KWAY; // _RB
     options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT; // _VOL
     options[METIS_OPTION_CTYPE] = METIS_CTYPE_SHEM; // _RM
     options[METIS_OPTION_IPTYPE] = METIS_IPTYPE_RANDOM; // _GROW _EDGE _NODE
     options[METIS_OPTION_RTYPE] = METIS_RTYPE_FM; // _GREEDY _SEP2SIDED _SEP1SIDED
     // options[METIS_OPTION_NCUTS] = 1;
     // options[METIS_OPTION_NITER] = 10;
     /* balance factor, used to be 500 */
     options[METIS_OPTION_UFACTOR] = 100;
     // options[METIS_OPTION_MINCONN];
     options[METIS_OPTION_CONTIG] = 1;
     // options[METIS_OPTION_SEED];
     options[METIS_OPTION_NUMBERING] = 0;
     // options[METIS_OPTION_DBGLVL] = 0;
 }
