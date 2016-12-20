#include "odmlem.h"

using namespace std;
using namespace lemon;
using namespace netxpert::core;

ODM_LEM_2Heap::ODM_LEM_2Heap(unsigned int nmx, unsigned int mmx, bool Drctd)
{
    //ctor
    isDrctd = Drctd;
}

void ODM_LEM_2Heap::ShortestPathTree()
{
    std::vector< SmartDigraph::Node > procList;

    while (!dijk->emptyQueue()) {
        SmartDigraph::Node n = dijk->processNextNode();
        cout << g.id(n) << " " <<dijk->dist(n) << endl;
    }

    /*for (auto d : this->dests)
        dijk->run(orig, d);*/


    /*while (!dijk->emptyQueue())
    {
        SmartDigraph::Node node = dijk->processNextNode();
        if (std::count(this->dests.begin(), this->dests.end(), node)>0)
        {
            std::cout << "Dest " << g.id(node)+1 << " processed"<< std::endl;
            procList.push_back(node);
        }*/

        /*
        if (procList.size() == this->dests.size())
        {
            std::cout << "Last Dest reached - Dijkstra stopped." << std::endl;
            break;
        }*/
    //}
}


void ODM_LEM_2Heap::LoadNet( unsigned int nmx , unsigned int mmx , unsigned int pn , unsigned int pm ,
		      double *pU , double *pC , double *pDfct ,
		      unsigned int *pSn , unsigned int *pEn )
{
	nmax = nmx;
	mmax = mmx;

    length = new SmartDigraph::ArcMap<double> (g);

    int origNode;
    int destNode;

    //Populate all nodes
    nodes.resize(nmax);
    for (unsigned int i = 0; i < nmax; ++i) {
      nodes[i] = g.addNode();
    }

    // construct arcs
    //depointerize
	SmartDigraph::ArcMap<double>& lengthVal = *length;
	double cost;
    SmartDigraph::Arc arc;

	if (isDrctd)
    {
		for (unsigned int i = 0; i < mmx; ++i) {
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
		for (unsigned int i = 0; i < mmx; ++i) {
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
	//init Dijkstra here
    dijk = new DijkstraInternal(g,lengthVal);
    dijk->init();
	//DijkstraInternal dijk(g,lengthVal);
}

void ODM_LEM_2Heap::SetOrigin( unsigned int NewOrg )
{
    //1. uint to Lemon Node
	orig = nodes[NewOrg-1];
	std::cout << "Adding new source "<< NewOrg << std::endl;
	dijk->addSource(orig);
}

void ODM_LEM_2Heap::SetDest( unsigned int NewDst )
{
    //1. uint to Lemon Node
	if (NewDst != UINT_MAX)
	{
		this->dests.push_back(nodes[NewDst-1]);
	}
	else
	{
		allDests = true;
	}
}
bool ODM_LEM_2Heap::Reached( unsigned int NodeID )
{
    //1. uint to Lemon Node
	SmartDigraph::Node node = nodes[NodeID-1];
    return dijk->reached(node);
}

/**< Return a cIndex* vector a[] such that a[ i ] is the index of the arc
   ( p[ i ] , i ), being p[] the vector returned by the above method, and
   with the same structure. If p[ i ] == 0, then a[ i ] is not significative:
   for the Origin (that has p[ Origin ] == 0), however, it is guaranteed that
   a[ Origin ] == Inf<Index>(). */

unsigned int* ODM_LEM_2Heap::ArcPredecessors( void )
{
	arcPredecessors = new unsigned int [MCFnmax()+1];
	arcPredecessors[0] = 0; //first entry of pred has no predecessor

    for (int i = 1; i < MCFnmax()+1; i++)
    {
        arcPredecessors[i] = g.id(dijk->predArc(nodes[i-1]));
    }
    return arcPredecessors;
}

/**< Return a cIndex* vector p[] such that p[ i ] is the predecessor of node
   i in the shortest path tree. If a node i has no predecessor, i.e.,
   i == Origin, i does not belong to the connected component of the origin or
   the computation have been stopped before reaching i, then p[ i ] == 0. */

unsigned int* ODM_LEM_2Heap::Predecessors( void )
{
	predecessors = new unsigned int [MCFnmax()+1];
	predecessors[0] = 0; //first entry of pred has no predecessor
    for (int i = 1; i < MCFnmax()+1; i++)
    {
        predecessors[i] = g.id(dijk->predNode(nodes[i-1])) + 1;
    }
	return predecessors;
}

void ODM_LEM_2Heap::GetArcPredecessors( unsigned int *outArcPrd )
{
	unsigned int size = MCFnmax()+1;
	auto arcPredecessors = ArcPredecessors();
	memcpy(outArcPrd, arcPredecessors, size * sizeof (unsigned int ) );
	delete[] arcPredecessors;
}

void ODM_LEM_2Heap::GetPredecessors( unsigned int *outPrd )
{
	unsigned int size = MCFnmax()+1;
	auto predecessors = Predecessors();
	memcpy(outPrd, predecessors, size * sizeof (unsigned int ) );
	delete[] predecessors;
}
std::vector<std::pair<unsigned int, unsigned int>>
 ODM_LEM_2Heap::GetPath ( unsigned int s, unsigned int t )
{
    std::vector<std::pair<unsigned int, unsigned int>> result;

    //id to lemon node
    SmartDigraph::Node pathSrc = nodes[s-1];
    SmartDigraph::Node pathDest = nodes[t-1];
    Path<SmartDigraph> path = dijk->path(pathDest);
    std::cout << "Path length:  "<< path.length() <<std::endl;

    std::cout << "Dist: " << dijk->currentDist(pathDest) << std::endl;
    /*
    for (int i = 0; i < path.length(); i++)
    //for (Path<SmartDigraph>::ArcIt a(path); a != INVALID; a++)
    {
        result.push_back( make_pair(g.id(g.source(path.nth(i))), g.id(g.target(path.nth(i)))) ); // gets the target node of nth arc of the path
        std::cout << "From: " << g.id(g.source(path.nth(i))) + 1 << " To: " << g.id(g.target(path.nth(i))) + 1 << std::endl;
    }*/

    for (SmartDigraph::Node v = pathDest; v != pathSrc; v = dijk->predNode(v)) {
        std::cout << g.id(v) + 1 << "<-";
    }
    std::cout << g.id(pathSrc) << std::endl;

	return result;
}


unsigned int ODM_LEM_2Heap::MCFnmax( void)
{
	return ( this->nmax );
}

unsigned int ODM_LEM_2Heap::MCFmmax( void)
{
	return ( this->mmax );
}

ODM_LEM_2Heap::~ODM_LEM_2Heap()
{
    //dtor
    delete dijk;
	delete length;
}
