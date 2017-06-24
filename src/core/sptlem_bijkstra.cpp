#include "sptlem_bijkstra.h"
#include <vector>

using namespace std;
using namespace lemon;
using namespace netxpert::core;

SPT_LEM_Bijkstra_2Heap::SPT_LEM_Bijkstra_2Heap(uint32_t nmx, uint32_t mmx , bool Drctd)
{
	//nmax = 0;
	//mmax = 0;
	isDrctd = Drctd;
	allDests = true; //unless set with SetDest()
}

void SPT_LEM_Bijkstra_2Heap::ShortestPathTree()
{
	if (allDests)
	{
        bijk->run(orig);//no bidirectional search; plain dijkstra
	}
	else
	{
        bijk->run(orig,dest);
	}
}
void SPT_LEM_Bijkstra_2Heap::LoadNet( uint32_t nmx , uint32_t mmx , uint32_t pn , uint32_t pm ,
		      double *pU , double *pC , double *pDfct ,
		      uint32_t *pSn , uint32_t *pEn )
{
	nmax = nmx;
	mmax = mmx;

    distMap = new SmartDigraph::NodeMap<double> (g);
    length = new SmartDigraph::ArcMap<double> (g);

    int origNode;
    int destNode;

    //Populate all nodes
    //std::vector<typename SmartDigraph::Node> nodes;
    nodes.resize(nmax);
    for (uint32_t i = 0; i < nmax; ++i) {
      nodes[i] = g.addNode();
    }

    // construct arcs
    //depointerize
	SmartDigraph::ArcMap<double>& lengthVal = *length;
	double cost;
    SmartDigraph::Arc arc;

	if (isDrctd)
    {
		for (uint32_t i = 0; i < mmx; ++i) {
			origNode = pSn[i];
			destNode = pEn[i];
			cost = pC[i];

			arc = g.addArc(nodes[origNode-1],nodes[destNode-1]);
			lengthVal[arc] = cost;
			//cout << "Arc: #" << i << " " << origNode << " " << destNode << " " << cost <<  endl;
		}
	}
	else // both directions
	{
		for (uint32_t i = 0; i < mmx; ++i) {
			origNode = pSn[i];
			destNode = pEn[i];
			cost = pC[i];
			arc = g.addArc(nodes[origNode-1],nodes[destNode-1]);
			lengthVal[arc] = cost;
			//cout << "Arc: #" << i << " " << origNode << " " << destNode << " " << cost <<  endl;
			arc = g.addArc(nodes[destNode-1],nodes[origNode-1]);
			lengthVal[arc] = cost;
			//cout << "Arc: #" << i << " " << destNode << " " << origNode << " " << cost <<  endl;
		}
    }
	//init Bijkstra here
    bijk = new BijkstraInternal(g, lengthVal);
}

void SPT_LEM_Bijkstra_2Heap::SetOrigin( uint32_t NewOrg )
{
    //1. uint to Lemon Node
	orig = nodes[NewOrg-1];
}

void SPT_LEM_Bijkstra_2Heap::SetDest( uint32_t NewDst )
{
    //1. uint to Lemon Node
	if (NewDst != UINT_MAX)
	{
		dest = nodes[NewDst-1];
	}
	else
	{
		allDests = true;
	}
}

bool SPT_LEM_Bijkstra_2Heap::Reached( uint32_t NodeID )
{
    //1. uint to Lemon Node
	SmartDigraph::Node node = nodes[NodeID-1];
    return bijk->reached(node);
}

void SPT_LEM_Bijkstra_2Heap::GetPath ( uint32_t Dst, uint32_t *outSn, uint32_t *outEn )
{
	if (!allDests)
	{
		//id to lemon node
		SmartDigraph::Node pathDest = nodes[Dst-1];
		Path<SmartDigraph> path = bijk->path(pathDest);

		for (int i = 0; i < path.length(); i++)
		/*for (Path<SmartDigraph>::ArcIt a(path); a != INVALID; a++)*/
		{
			outSn[i] = g.id(g.source(path.nth(i))); // gets the source node of nth arc of the path
			outEn[i] = g.id(g.target(path.nth(i))); // gets the target node of nth arc of the path
			//cout << "From: " << g.id(g.source(path.nth(i))) + 1 << " To: " << g.id(g.target(path.nth(i))) + 1 << endl;
		}
	}
}


/**< Return a cIndex* vector a[] such that a[ i ] is the index of the arc
   ( p[ i ] , i ), being p[] the vector returned by the above method, and
   with the same structure. If p[ i ] == 0, then a[ i ] is not significative:
   for the Origin (that has p[ Origin ] == 0), however, it is guaranteed that
   a[ Origin ] == Inf<Index>(). */

uint32_t* SPT_LEM_Bijkstra_2Heap::ArcPredecessors( void )
{
	arcPredecessors = new uint32_t [MCFnmax()+1];
	arcPredecessors[0] = 0; //first entry of pred has no predecessor

    for (int i = 1; i < MCFnmax()+1; i++)
    {
        arcPredecessors[i] = g.id(bijk->predArc(nodes[i-1]));
    }

	return arcPredecessors;
}

/**< Return a cIndex* vector p[] such that p[ i ] is the predecessor of node
   i in the shortest path tree. If a node i has no predecessor, i.e.,
   i == Origin, i does not belong to the connected component of the origin or
   the computation have been stopped before reaching i, then p[ i ] == 0. */

uint32_t* SPT_LEM_Bijkstra_2Heap::Predecessors( void )
{
	predecessors = new uint32_t [MCFnmax()+1];
	predecessors[0] = 0; //first entry of pred has no predecessor
    for (int i = 1; i < MCFnmax()+1; i++)
    {
        predecessors[i] = g.id(bijk->predNode(nodes[i-1])) + 1;
    }

	return predecessors;
}

void SPT_LEM_Bijkstra_2Heap::GetArcPredecessors( uint32_t *outArcPrd )
{
	uint32_t size = MCFnmax()+1;
	auto arcPredecessors = ArcPredecessors();
	memcpy(outArcPrd, arcPredecessors, size * sizeof (uint32_t ) );
	delete[] arcPredecessors;
}

void SPT_LEM_Bijkstra_2Heap::GetPredecessors( uint32_t *outPrd )
{
	uint32_t size = MCFnmax()+1;
	auto predecessors = Predecessors();
	memcpy(outPrd, predecessors, size * sizeof (uint32_t ) );
	delete[] predecessors;
}

uint32_t SPT_LEM_Bijkstra_2Heap::MCFnmax( void)
{
	return ( nmax );
}

uint32_t SPT_LEM_Bijkstra_2Heap::MCFmmax( void)
{
	return ( mmax );
}

void SPT_LEM_Bijkstra_2Heap::PrintResult( void )
{
//    SmartDigraph::NodeMap<double> d = dijk->distMap();
    //depointerize distMap
    SmartDigraph::NodeMap<double>& distMapVal = *distMap;
	cout << "***********************************************" << endl;
	cout << "C++ Output START" << endl;
	if (!allDests)
	{
		cout << "Dest reached: " << bijk->reached(dest) << endl;
	}
    cout << "Orig reached: " << bijk->reached(orig) << endl;
	if (!allDests)
	{
		cout << "Path cost to dest: " << distMapVal[dest] << endl;
	}
	cout << "C++ Output END" << endl;
	cout << "***********************************************" << endl;
    //cout << "Path cost to orig: " << distMapVal[orig] << endl;
    //printf("%i\n", distMapVal[start]);
}

SPT_LEM_Bijkstra_2Heap::~SPT_LEM_Bijkstra_2Heap()
{
    //dtor
    delete bijk;
    delete distMap;
	delete length;
}
