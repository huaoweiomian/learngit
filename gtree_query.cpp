// macro for 64 bits file, larger than 2G
#define _FILE_OFFSET_BITS 64


#include "gtree.h"

namespace query {

// input init
void init_input(){
	FILE *fin;

	// load node
	printf("LOADING NODE...");
	fin = fopen(FILE_NODE,"r");
	int nid;
	double x,y;
	while( fscanf(fin, "%d %lf %lf", &nid, &x, &y ) == 3 ){
		Node node = { x, y };
		Nodes.push_back(node);
	}
	fclose(fin);
	printf("COMPLETE. NODE_COUNT=%d\n", (int)Nodes.size());

	// load edge
	printf("LOADING EDGE...");
	fin = fopen(FILE_EDGE, "r");
	int eid;
	int snid, enid;
	double weight;
	int iweight;
	noe = 0;
	while( fscanf(fin,"%d %d %d %lf", &eid, &snid, &enid, &weight ) == 4 ){
		noe ++;
		iweight = (int) (weight * WEIGHT_INFLATE_FACTOR );
		Nodes[snid].adjnodes.push_back( enid );
		Nodes[snid].adjweight.push_back( iweight );
		Nodes[enid].adjnodes.push_back( snid );
		Nodes[enid].adjweight.push_back( iweight );
	}
	fclose(fin);
	printf("COMPLETE.\n");
}

void init(){
	init_input();
	options_setting();
}

void finalize(){
    delete xadj;
    delete adjncy;
    delete adjwgt;
    delete part;
}

// load gtree index from file
void gtree_load(){
	// FILE_GTREE
	FILE *fin = fopen( FILE_GTREE, "rb" );
	int *buf = new int[ Nodes.size() ];
	int count_borders, count_children, count_leafnodes;
	bool isleaf;
	int father;

	// clear gtree
	GTree.clear();

	while( fread( &count_borders, sizeof(int), 1, fin ) ){
		TreeNode tn;
		// borders
		tn.borders.clear();
		fread( buf, sizeof(int), count_borders, fin );
		for ( int i = 0; i < count_borders; i++ ){
			tn.borders.push_back(buf[i]);
		}
		// children
		fread( &count_children, sizeof(int), 1, fin );
		fread( buf, sizeof(int), count_children, fin );
		for ( int i = 0; i < count_children; i++ ){
			tn.children.push_back(buf[i]);
		}
		// isleaf
		fread( &isleaf, sizeof(bool), 1, fin );
		tn.isleaf = isleaf;
		// leafnodes
		fread( &count_leafnodes, sizeof(int), 1, fin );
		fread( buf, sizeof(int), count_leafnodes, fin );
		for ( int i = 0; i < count_leafnodes; i++ ){
			tn.leafnodes.push_back(buf[i]);
		}
		// father
		fread( &father, sizeof(int), 1, fin );
		tn.father = father;

		GTree.push_back(tn);
	}
	fclose(fin);
	
	// FILE_NODES_GTREE_PATH
	int count;
	fin = fopen( FILE_NODES_GTREE_PATH, "rb" );
	int pos = 0;
	while( fread( &count, sizeof(int), 1, fin ) ){
		fread( buf, sizeof(int), count, fin );
		// clear gtreepath
		Nodes[pos].gtreepath.clear();
		for ( int i = 0; i < count; i++ ){
			Nodes[pos].gtreepath.push_back( buf[i] );
		}
		// pos increase
		pos ++;
	}
	fclose(fin);
	delete[] buf;
}

// dijkstra search, used for single-source shortest path search WITHIN one gtree leaf node!
// input: s = source node
//        cands = candidate node list
//        graph = search graph(this can be set to subgraph)
vector<int> dijkstra_candidate( int s, vector<int> &cands, vector<Node> &graph ){
	// init
	set<int> todo;
	todo.clear();
	todo.insert(cands.begin(), cands.end());
	
	unordered_map<int,int> result;
	result.clear();
	set<int> visited;
	visited.clear();
	unordered_map<int,int> q;
	q.clear();
	q[s] = 0;

	// start
	int min, minpos, adjnode, weight;
	while( ! todo.empty() && ! q.empty() ){
		min = -1;
		for ( unordered_map<int,int>::iterator it = q.begin(); it != q.end(); it ++ ){
			if ( min == -1 ){
				minpos = it -> first;
				min = it -> second;
			}
			else{
				if ( it -> second < min ){
					min = it -> second;
					minpos = it -> first;
				}
			}
		}

		// put min to result, add to visited
		result[minpos] = min;
		visited.insert( minpos );
		q.erase(minpos);

		if ( todo.find( minpos ) != todo.end() ){
			todo.erase( minpos );
		}

		// expand
		for ( int i = 0; i < graph[minpos].adjnodes.size(); i++ ){
			adjnode = graph[minpos].adjnodes[i];
			if ( visited.find( adjnode ) != visited.end() ){
				continue;
			}
			weight = graph[minpos].adjweight[i];

			if ( q.find(adjnode) != q.end() ){
				if ( min + weight < q[adjnode] ){
					q[adjnode] = min + weight;
				}
			}
			else{
				q[adjnode] = min + weight;
			}
		
		}
	}

	// output
	vector<int> output;
	for ( int i = 0; i < cands.size(); i++ ){
		output.push_back( result[cands[i]] );
	}

	// return
	return output;
}

// load distance matrix from file
void hierarchy_shortest_path_load(){
	FILE* fin = fopen( FILE_ONTREE_MIND, "rb" );
	int* buf;
	int count, pos = 0;
	while( fread( &count, sizeof(int), 1, fin ) ){
		// union borders
		buf = new int[count];
		fread( buf, sizeof(int), count, fin );
		GTree[pos].union_borders.clear();
		for ( int i = 0; i < count; i++ ){
			GTree[pos].union_borders.push_back(buf[i]);
		}
		delete[] buf;
		// mind
		fread( &count, sizeof(int), 1, fin );
		buf = new int[count];
		fread( buf, sizeof(int), count, fin );
		GTree[pos].mind.clear();
		for ( int i = 0; i < count; i++ ){
			GTree[pos].mind.push_back(buf[i]);
		}
		pos++;
		delete[] buf;
	}
	fclose(fin);
}

// before query, we have to set OCCURENCE LIST etc.
// this is done only ONCE for a given set of objects.
void pre_query(){
	// first clear all
	for ( int i = 0; i < GTree.size(); i++ ){
		GTree[i].nonleafinvlist.clear();
		GTree[i].leafinvlist.clear();
	}
	
	// read object list
	vector<int> o;
	o.clear();

	char file_object[100];
	sprintf( file_object, "%s", FILE_OBJECT );
	FILE *fin = fopen( file_object, "r" );
	int oid, id;
	while( fscanf( fin, "%d %d", &oid, &id ) == 2 ){
		o.push_back(oid);
	}
	fclose(fin);

	// set OCCURENCE LIST
	for ( int i = 0; i < o.size(); i++ ){
		int current = Nodes[o[i]].gtreepath.back();
		// add leaf inv list
		int pos = lower_bound( GTree[current].leafnodes.begin(), GTree[current].leafnodes.end(), o[i] ) - GTree[current].leafnodes.begin();
		GTree[current].leafinvlist.push_back(pos);		
		// recursive
		int child;
		while( current != -1 ){
			child = current;
			current = GTree[current].father;
			if ( current == -1 ) break;
			if ( find(GTree[current].nonleafinvlist.begin(), GTree[current].nonleafinvlist.end(), child ) == GTree[current].nonleafinvlist.end() ){
				GTree[current].nonleafinvlist.push_back(child);
			}
		}
	}

	// up_pos & current_pos(used for quickly locating parent & child nodes)
	unordered_map<int,int> pos_map;
	for ( int i = 1; i < GTree.size(); i++ ){
		GTree[i].current_pos.clear();
		GTree[i].up_pos.clear();

		// current_pos
		pos_map.clear();
		for ( int j = 0; j < GTree[i].union_borders.size(); j++ ){
			pos_map[GTree[i].union_borders[j]] = j;
		}
		for ( int j = 0; j < GTree[i].borders.size(); j++ ){
			GTree[i].current_pos.push_back(pos_map[GTree[i].borders[j]]);
		}
		// up_pos
		pos_map.clear();
		for ( int j = 0; j < GTree[GTree[i].father].union_borders.size(); j++ ){
			pos_map[GTree[GTree[i].father].union_borders[j]] = j;
		}
		for ( int j = 0; j < GTree[i].borders.size(); j++ ){
			GTree[i].up_pos.push_back(pos_map[GTree[i].borders[j]]);
		}
	}
}
void pre_query_web(int enda){
    vector<int> o;
    o.push_back(enda);
    // first clear all
    for ( int i = 0; i < GTree.size(); i++ ){
        GTree[i].nonleafinvlist.clear();
        GTree[i].leafinvlist.clear();
    }


    // set OCCURENCE LIST
    for ( int i = 0; i < o.size(); i++ ){
        int current = Nodes[o[i]].gtreepath.back();
        // add leaf inv list
        int pos = lower_bound( GTree[current].leafnodes.begin(), GTree[current].leafnodes.end(), o[i] ) - GTree[current].leafnodes.begin();
        GTree[current].leafinvlist.push_back(pos);
        // recursive
        int child;
        while( current != -1 ){
            child = current;
            current = GTree[current].father;
            if ( current == -1 ) break;
            if ( find(GTree[current].nonleafinvlist.begin(), GTree[current].nonleafinvlist.end(), child ) == GTree[current].nonleafinvlist.end() ){
                GTree[current].nonleafinvlist.push_back(child);
            }
        }
    }

    // up_pos & current_pos(used for quickly locating parent & child nodes)
    unordered_map<int,int> pos_map;
    for ( int i = 1; i < GTree.size(); i++ ){
        GTree[i].current_pos.clear();
        GTree[i].up_pos.clear();

        // current_pos
        pos_map.clear();
        for ( int j = 0; j < GTree[i].union_borders.size(); j++ ){
            pos_map[GTree[i].union_borders[j]] = j;
        }
        for ( int j = 0; j < GTree[i].borders.size(); j++ ){
            GTree[i].current_pos.push_back(pos_map[GTree[i].borders[j]]);
        }
        // up_pos
        pos_map.clear();
        for ( int j = 0; j < GTree[GTree[i].father].union_borders.size(); j++ ){
            pos_map[GTree[GTree[i].father].union_borders[j]] = j;
        }
        for ( int j = 0; j < GTree[i].borders.size(); j++ ){
            GTree[i].up_pos.push_back(pos_map[GTree[i].borders[j]]);
        }
    }
}

// init search node
typedef struct{
	int id;
	bool isvertex;
	int lca_pos;
	int dis;
}Status_query;

struct Status_query_comp{
	bool operator()( const Status_query& l, const Status_query& r ){
		return l.dis > r.dis;
	}
};

typedef struct{
	int id;
	int dis;
    vector<int> paths;
}ResultSet;

// ----- CORE PART -----
// knn search
// input: locid = query location, node id
//        K = top-K
// output: a vector of ResultSet, each is a tuple (node id, shortest path), ranked by shortest path distance from query location
vector<ResultSet> knn_query( int locid, int K ){
	// init priority queue & result set
	vector<Status_query> pq;
	pq.clear();
	vector<ResultSet> rstset;
	rstset.clear();

	// init upstream
	unordered_map<int, vector<int> > itm; // intermediate answer, tree node -> array
	itm.clear();
	int tn, cid, posa, posb, min, dis;
	for ( int i = Nodes[locid].gtreepath.size() - 1; i > 0; i-- ){
		tn = Nodes[locid].gtreepath[i];
		itm[tn].clear();

		if ( GTree[tn].isleaf ){
			posa = lower_bound( GTree[tn].leafnodes.begin(), GTree[tn].leafnodes.end(), locid ) - GTree[tn].leafnodes.begin();

			for ( int j = 0; j < GTree[tn].borders.size(); j++ ){
				itm[tn].push_back( GTree[tn].mind[ j * GTree[tn].leafnodes.size() + posa ] );
			}
		}
		else{
			cid = Nodes[locid].gtreepath[i+1];
			for ( int j = 0; j < GTree[tn].borders.size(); j++ ){
				min = -1;
				posa = GTree[tn].current_pos[j];
				for ( int k = 0; k < GTree[cid].borders.size(); k++ ){
					posb = GTree[cid].up_pos[k];
					dis = itm[cid][k] + GTree[tn].mind[ posa * GTree[tn].union_borders.size() + posb ];
					// get min
					if ( min == -1 ){
						min = dis;
					}
					else{
						if ( dis < min ){
							min = dis;
						}
					}
				}
				// update
				itm[tn].push_back( min );
			}
		}

	}

	// do search
	Status_query rootstatus = { 0, false, 0, 0 };
	pq.push_back( rootstatus );
	make_heap( pq.begin(), pq.end(), Status_query_comp() );

	vector<int> cands, result;
	int child, son, allmin, vertex;

	while( pq.size() > 0 && rstset.size() < K ){
		Status_query top = pq[0];
		pop_heap( pq.begin(), pq.end(), Status_query_comp() );
		pq.pop_back();

		if ( top.isvertex ){
			ResultSet rs = { top.id, top.dis };
			rstset.push_back(rs);
		}
		else{
			if ( GTree[top.id].isleaf ){
				// inner of leaf node, do dijkstra
				if ( top.id == Nodes[locid].gtreepath[top.lca_pos] ){
					
					cands.clear();
					for ( int i = 0; i < GTree[top.id].leafinvlist.size(); i++ ){
						cands.push_back( GTree[top.id].leafnodes[GTree[top.id].leafinvlist[i]] );
					}
					result = dijkstra_candidate( locid, cands, Nodes );
					for ( int i = 0; i < cands.size(); i++ ){
						Status_query status = { cands[i], true, top.lca_pos, result[i] };
						pq.push_back(status);
						push_heap( pq.begin(), pq.end(), Status_query_comp() );
					}
					
				}
	
				// else do 
				else{
					for ( int i = 0; i < GTree[top.id].leafinvlist.size(); i++ ){
						posa = GTree[top.id].leafinvlist[i];
						vertex = GTree[top.id].leafnodes[posa];
						allmin = -1;

						for ( int k = 0; k < GTree[top.id].borders.size(); k++ ){
							dis = itm[top.id][k] + GTree[top.id].mind[ k * GTree[top.id].leafnodes.size() + posa ];
							if ( allmin == -1 ){
								allmin = dis;
							}
							else{
								if ( dis < allmin ){
									allmin = dis;
								}
							}

						}
						
						Status_query status = { vertex, true, top.lca_pos, allmin };
						pq.push_back(status);
						push_heap( pq.begin(), pq.end(), Status_query_comp() );

					}
				}
			}
			else{
				for ( int i = 0; i < GTree[top.id].nonleafinvlist.size(); i++ ){
					child = GTree[top.id].nonleafinvlist[i];
					son = Nodes[locid].gtreepath[ top.lca_pos + 1 ];
					// on gtreepath
					if ( child == son ){
						Status_query status = { child, false, top.lca_pos + 1, 0 };
						pq.push_back(status);
						push_heap( pq.begin(), pq.end(), Status_query_comp() );
					}
					// brothers
					else if ( GTree[child].father == GTree[son].father ){
						itm[child].clear();
						allmin = -1;

						for ( int j = 0; j < GTree[child].borders.size(); j++ ){
							min = -1;
							posa = GTree[child].up_pos[j];
							for( int k = 0; k < GTree[son].borders.size(); k++ ){
								posb = GTree[son].up_pos[k];
								dis = itm[son][k] + GTree[top.id].mind[ posa * GTree[top.id].union_borders.size() + posb ];
								if ( min == -1 ){
									min = dis;
								}
								else{
									if ( dis < min ){
										min = dis;
									}
								}
							}
							itm[child].push_back(min);	
							// update all min
							if ( allmin == -1 ){
								allmin = min;
							}
							else if ( min < allmin ){
								allmin = min;
							}
						}
						Status_query status = { child, false, top.lca_pos, allmin };
						pq.push_back(status);
						push_heap( pq.begin(), pq.end(), Status_query_comp() );
					}
					// downstream
					else{
						itm[child].clear();
						allmin = -1;
						
						for ( int j = 0; j < GTree[child].borders.size(); j++ ){
							min = -1;
							posa = GTree[child].up_pos[j];
							for ( int k = 0; k < GTree[top.id].borders.size(); k++ ){
								posb = GTree[top.id].current_pos[k];
								dis = itm[top.id][k] + GTree[top.id].mind[ posa * GTree[top.id].union_borders.size() + posb ];
								if ( min == -1 ){
									min = dis;
								}
								else{
									if ( dis < min ){
										min = dis;
									}
								}
							}
							itm[child].push_back(min);
							// update all min
							if ( allmin == -1 ){
								allmin = min;
							}
							else if ( min < allmin ){
								allmin = min;
							}
						}
						Status_query status = { child, false, top.lca_pos, allmin };
                        pq.push_back(status);
                        push_heap( pq.begin(), pq.end(), Status_query_comp() );
					}
				}
			}
			
		}
	}

	/* ----- return id list -----
	vector<int> rst;
	for ( int i = 0; i < rstset.size(); i++ ){
		rst.push_back( rstset[i].id) ;
	}
	
	return rst;
	*/

	return rstset;
}
void mindpath_load(){
    int pos = 0;
    FILE* fin = fopen( FILE_MINDS_PATH, "rb");
    while (fread(&pos,sizeof(int),1, fin)){
        int size = 0;
        fread(&size,sizeof(int),1, fin);
        for (int i = 0; i < size; ++i){
            int num = 0;
            fread(&num,sizeof(int),1, fin);
            int* buf = new int[num];
            fread( buf, sizeof(int), num, fin );
            vector<int> tmp(buf, buf+num);
            GTree[pos].paths.push_back(tmp);
            delete[] buf;
        }
    }
    fclose(fin);
}
int qmain(){
	// init
	TIME_TICK_START
	init();
	TIME_TICK_END
	TIME_TICK_PRINT("INIT")

	// load gtree index
	gtree_load();

	// load distance matrix
	hierarchy_shortest_path_load();
    mindpath_load();
	// pre query init


	// knn search
	// example
	printf("KNN Search Started...\n");
	int locid, K;
	vector<ResultSet> result;
	while(scanf("%d %d", &locid, &K) == 2){
		if (locid >= Nodes.size() || locid < 0 || K < 0 || K > Nodes.size()) continue;
        pre_query_web(K);
		TIME_TICK_START
        result = knn_query(locid, 1);
		TIME_TICK_END
		for ( int i = 0; i < result.size(); i++ ){
			printf("ID=%d DIS=%d\n", result[i].id, result[i].dis );
		}
		TIME_TICK_PRINT("KNN_SEARCH")
	}

	return 0;
}
}
