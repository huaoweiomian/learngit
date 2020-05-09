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
    Nodes.clear();
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

    //记录终点所在支路
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
    unordered_map<int,int> pos_map;//在最短路径矩阵中的查找最短路径
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
    vector<int> path;
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
    //处理路径，除掉多余的点，如果有需要调整路径顺序
    auto _get_path = [](vector<int> start, vector<int> patha)->vector<int> {
        if (patha.empty()){
            return start;
        }
        if(patha.back() != start.back()){
            auto bg = patha.begin();
            if(start.back() == patha[0])
                bg++;
            start.insert(start.end(),bg, patha.end());
            return start;
        }
        reverse(patha.begin(),patha.end());
        start.insert(start.end(),patha.begin()+1, patha.end());
        return start;
    };
	// init priority queue & result set
	vector<Status_query> pq;
	pq.clear();
	vector<ResultSet> rstset;
	rstset.clear();

	// init upstream
	unordered_map<int, vector<int> > itm; // intermediate answer, tree node -> array
    unordered_map<int, vector< vector<int> > > paths;
	itm.clear();
	int tn, cid, posa, posb, min, dis;
    //算出从起点开始到gtree根节点前一个节点的最短路径
	for ( int i = Nodes[locid].gtreepath.size() - 1; i > 0; i-- ){
		tn = Nodes[locid].gtreepath[i];
		itm[tn].clear();
        //记录同一个子节点的最短路径
		if ( GTree[tn].isleaf ){
			posa = lower_bound( GTree[tn].leafnodes.begin(), GTree[tn].leafnodes.end(), locid ) - GTree[tn].leafnodes.begin();

			for ( int j = 0; j < GTree[tn].borders.size(); j++ ){
				itm[tn].push_back( GTree[tn].mind[ j * GTree[tn].leafnodes.size() + posa ] );
                auto tmp = GTree[tn].paths[ j * GTree[tn].leafnodes.size() + posa ];
                reverse(tmp.begin(),tmp.end());
                paths[tn].push_back(tmp);
			}
		}
		else{
            //计算并记录到达父图border的最短路径
			cid = Nodes[locid].gtreepath[i+1];
			for ( int j = 0; j < GTree[tn].borders.size(); j++ ){
                vector<int> pathmin;
				min = -1;
				posa = GTree[tn].current_pos[j];
				for ( int k = 0; k < GTree[cid].borders.size(); k++ ){
					posb = GTree[cid].up_pos[k];
                    vector<int> tmp = _get_path(paths[cid][k], GTree[tn].paths[ posa * GTree[tn].union_borders.size() + posb ]);
					dis = itm[cid][k] + GTree[tn].mind[ posa * GTree[tn].union_borders.size() + posb ];
					// get min

					if ( min == -1 ){
						min = dis;
                        pathmin = tmp;
					}
					else{
						if ( dis < min ){
							min = dis;
                            pathmin = tmp;
						}
					}
				}
				// update
				itm[tn].push_back( min );
                paths[tn].push_back(pathmin);
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

        if ( top.isvertex ){//找到终点
            ResultSet rs = { top.id, top.dis,top.path };
			rstset.push_back(rs);
		}
		else{
            if ( GTree[top.id].isleaf ){//当前正在处理的gtree节点是叶子节点
				// inner of leaf node, do dijkstra
				if ( top.id == Nodes[locid].gtreepath[top.lca_pos] ){
                    //在同一个子图，使用dijkstra算出最短路径
					cands.clear();
					for ( int i = 0; i < GTree[top.id].leafinvlist.size(); i++ ){
						cands.push_back( GTree[top.id].leafnodes[GTree[top.id].leafinvlist[i]] );
					}
                    //result = dijkstra_candidate( locid, cands, Nodes );
                    unordered_map<int, int> tmppaths;
                    result = build::dijkstra_candidate_path(locid, cands, Nodes, tmppaths);
					for ( int i = 0; i < cands.size(); i++ ){
                        Status_query status = { cands[i], true, top.lca_pos, result[i], build::get_paths(locid,cands[i], tmppaths) };
						pq.push_back(status);
						push_heap( pq.begin(), pq.end(), Status_query_comp() );
					}
					
				}
	
				// else do 
                else{//不在同一个子图,算出终点到border的最短路径，再加上之前记录的到子图的最短路径
					for ( int i = 0; i < GTree[top.id].leafinvlist.size(); i++ ){
						posa = GTree[top.id].leafinvlist[i];
						vertex = GTree[top.id].leafnodes[posa];
						allmin = -1;
                        vector<int> allpathmin;
						for ( int k = 0; k < GTree[top.id].borders.size(); k++ ){
                            vector<int> tmp = _get_path(paths[top.id][k], GTree[top.id].paths[  k * GTree[top.id].leafnodes.size() + posa  ]);
							dis = itm[top.id][k] + GTree[top.id].mind[ k * GTree[top.id].leafnodes.size() + posa ];
							if ( allmin == -1 ){
								allmin = dis;
                                allpathmin = tmp;
							}
							else{
								if ( dis < allmin ){
									allmin = dis;
                                    allpathmin = tmp;
								}
							}

						}
						
                        Status_query status = { vertex, true, top.lca_pos, allmin,allpathmin };
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
                    if ( child == son ){//在同一支路，沿支路向下走
                        Status_query status = { child, false, top.lca_pos + 1, 0,vector<int>() };
						pq.push_back(status);
						push_heap( pq.begin(), pq.end(), Status_query_comp() );
					}
					// brothers
                    else if ( GTree[child].father == GTree[son].father ){//兄弟节点
						itm[child].clear();
                        paths[child].clear();
                        vector<int> allpathmin;
						allmin = -1;
                        //算出son子图到child子图的最短路径，再加上之前的最短路径得到当前最短路径
						for ( int j = 0; j < GTree[child].borders.size(); j++ ){
                            vector<int> pathmin;
                            min = -1;
							posa = GTree[child].up_pos[j];
							for( int k = 0; k < GTree[son].borders.size(); k++ ){
								posb = GTree[son].up_pos[k];
								dis = itm[son][k] + GTree[top.id].mind[ posa * GTree[top.id].union_borders.size() + posb ];
                                vector<int> tmp = _get_path(paths[son][k],GTree[top.id].paths[ posa * GTree[top.id].union_borders.size() + posb ]);
								if ( min == -1 ){
									min = dis;
                                    pathmin = tmp;
								}
								else{
									if ( dis < min ){
										min = dis;
                                        pathmin = tmp;
									}
								}
							}
							itm[child].push_back(min);	
                            paths[child].push_back(pathmin);
							// update all min
							if ( allmin == -1 ){
								allmin = min;
                                allpathmin = pathmin;
							}
							else if ( min < allmin ){
								allmin = min;
                                allpathmin = pathmin;
							}
						}
                        Status_query status = { child, false, top.lca_pos, allmin, allpathmin };
						pq.push_back(status);
						push_heap( pq.begin(), pq.end(), Status_query_comp() );
					}
					// downstream
                    else{//下游子图，算出父图到子图的最短路径，加上之前的最短路径
						itm[child].clear();
						allmin = -1;
                        vector<int> allpathmin;
						for ( int j = 0; j < GTree[child].borders.size(); j++ ){
							min = -1;
                            vector<int> pathmin;
							posa = GTree[child].up_pos[j];
							for ( int k = 0; k < GTree[top.id].borders.size(); k++ ){
								posb = GTree[top.id].current_pos[k];
								dis = itm[top.id][k] + GTree[top.id].mind[ posa * GTree[top.id].union_borders.size() + posb ];
                                vector<int> tmp = _get_path(paths[top.id][k], GTree[top.id].paths[ posa * GTree[top.id].union_borders.size() + posb ]);
								if ( min == -1 ){
									min = dis;
                                    pathmin = tmp;
								}
								else{
									if ( dis < min ){
										min = dis;
                                        pathmin = tmp;
									}
								}
							}
							itm[child].push_back(min);
                            paths[child].push_back(pathmin);
							// update all min
							if ( allmin == -1 ){
								allmin = min;
                                allpathmin = pathmin;
							}
							else if ( min < allmin ){
								allmin = min;
                                allpathmin = pathmin;
							}
						}
                        Status_query status = { child, false, top.lca_pos, allmin,allpathmin };
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
vector<int> search(int start, int end){
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
    auto fpt = [](vector<int> &vs){
        cout<<"path: ";
        for(auto v : vs){
            cout<<v<<" ";
        }
        cout<<endl;
    };
    // knn search
    // example
    printf("KNN Search Started...\n");
    int locid = start, K = end;
    vector<ResultSet> result;
    if (locid >= Nodes.size() || locid < 0 || K < 0 || K > Nodes.size())
        return vector<int>();
    pre_query_web(K);
    TIME_TICK_START
            result = knn_query(locid, 1);
    TIME_TICK_END
            for ( int i = 0; i < result.size(); i++ ){
        printf("ID=%d DIS=%d\n", result[i].id, result[i].dis );
        fpt(result[i].paths);
    }
    TIME_TICK_PRINT("KNN_SEARCH")
            return result[0].paths;
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
    auto fpt = [](vector<int> &vs){
        cout<<"path: ";
        for(auto v : vs){
            cout<<v<<" ";
        }
        cout<<endl;
    };

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
            fpt(result[i].paths);
		}
		TIME_TICK_PRINT("KNN_SEARCH")
	}

	return 0;
}
}
