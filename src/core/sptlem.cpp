#include "sptlem.h"

using namespace std;
using namespace lemon;
using namespace netxpert::core;

SPT_LEM_2Heap::SPT_LEM_2Heap( unsigned int nmx, unsigned int mmx , bool Drctd)
{
	//nmax = 0;
	//mmax = 0;
	isDrctd = Drctd;
	allDests = true; //unless set with SetDest()
}

void SPT_LEM_2Heap::ShortestPathTree()
{
	//dijk = new Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>>(g, lengthVal);
	/* Change Heap */

	/*SmartDigraph::NodeMap<double> heap_cross_ref(g);
	FibonacciHeap heap(heap_cross_ref);      */
	//dijk = new Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>, DijkstraDefaultTraits<SmartDigraph, SmartDigraph::ArcMap<double>>>::SetHeap<FibonacciHeap, SmartDigraph::NodeMap<double>> (g, lengthVal);
	//Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>>::SetHeap<FibonacciHeap, SmartDigraph::NodeMap<double>>::Create d(g, lengthVal);
	//dijk = &d;
	//
	//dijk->heap( heap, heap_cross_ref );

	//DijkstraInternal d(g,lengthVal);

	//dijk->heap(heap, heap_cross_ref);

	//Dijkstra<SmartDigraph, SmartDigraph::ArcMap<double>>::SetHeap<FibonacciHeap, SmartDigraph::NodeMap<double>>()

 //   dijk->distMap(distVal);
	//

	if (allDests)
	{
        dijk->run(orig);
	}
	else
	{
        dijk->run(orig,dest);
	}
	//if (allDests)
	//{
	//	d.run(orig);
	//}
	//else
	//{
	//	d.run(orig, dest);
	//}
	//Alternate to run():
	//dijk->init();
	//dijk->addSource(orig);
	//dijk->start();

	//delete dijk;
}

/*void SPT_LEM_2Heap::ShortestPathTree(double maxCutOff)
{
    dijk->init();
    dijk->addSource(orig);

    //break if max cut off value is reached
    double currentLength = 0;
    cout << "Max dist: "<<maxCutOff << endl;
    while ( !dijk->emptyQueue() )
    {
        if (currentLength > maxCutOff)
            continue; //process next node

        lemon::SmartDigraph::Node currNode = dijk->nextNode();
        dijk->start(currNode);

        cout << "Current dist: "<<currentLength << endl;
        //get length so far
        if (dijk->reached(currNode))
            currentLength += dijk->dist(currNode);
    }
    cout << "Exited at dist: "<<currentLength << endl;
}*/

void SPT_LEM_2Heap::LoadNet( unsigned int nmx , unsigned int mmx , unsigned int pn , unsigned int pm ,
		      double *pU , double *pC , double *pDfct ,
		      unsigned int *pSn , unsigned int *pEn )
{
	nmax = nmx;
	mmax = mmx;

	//smart graph
    distMap = new SmartDigraph::NodeMap<double> (g);
    length = new SmartDigraph::ArcMap<double> (g);

	//static graph
    //distMap = new StaticDigraph::NodeMap<double> (g);
    //length = new StaticDigraph::ArcMap<double> (g);

    int origNode;
    int destNode;

    //Populate all nodes
    //std::vector<typename SmartDigraph::Node> nodes;

    //smart graph
    nodes.resize(nmax);
    for (unsigned int i = 0; i < nmax; ++i) {
      nodes[i] = g.addNode();
    }

   //   if (pDfct[i] < 0)
	  //{
   //     cout << "Startnode "<< i+1 << endl;
   //     orig = nodes[i];
	  //}
   //   if (pDfct[i] > 0)
	  //{
   //     cout << "Endnode "<< i+1 << endl;
   //     dest = nodes[i];
	  //}
      //cout << "Node "<< i << endl;


    // construct arcs
    //depointerize
	SmartDigraph::ArcMap<double>& lengthVal = *length;
	//StaticDigraph::ArcMap<double>& lengthVal = *length;
	double cost;
    SmartDigraph::Arc arc;

    //std::vector<std::pair<int,int>> arcList(mmax);
    //isDrctd = true;
	if (isDrctd)
    {
        //unsigned int lastSrcId = 0;
		for (unsigned int i = 0; i < mmax; ++i) {
			origNode = pSn[i];
			destNode = pEn[i];
			cost = pC[i];

            /*if (origNode < lastSrcId)
            {
                std::cout << "ignored " << to_string(origNode)<< " < " << to_string(lastSrcId) << std::endl;
            }
            else*/
			arc = g.addArc(nodes[origNode-1],nodes[destNode-1]);
            //    arcList.push_back(std::make_pair( origNode, destNode));

            //lastSrcId = origNode;
			lengthVal[arc] = cost;

			//cout << "Arc: #" << i << " " << origNode << " " << destNode << " " << cost <<  endl;
		}
	}
	else // both directions
	{

		//std::cout << "\n BOTH DIRECTIONS "<< std::endl;

		for (unsigned int i = 0; i < mmax; ++i) {
			origNode = pSn[i];
			destNode = pEn[i];
			cost = pC[i];
			arc = g.addArc(nodes[origNode-1],nodes[destNode-1]);
			//arcList.push_back(std::make_pair(origNode, destNode));
			lengthVal[arc] = cost;
			//cout << "Arc: #" << i << " " << origNode << " " << destNode << " " << cost <<  endl;
			arc = g.addArc(nodes[destNode-1],nodes[origNode-1]);
			//arcList.push_back(std::make_pair(destNode, origNode));
			lengthVal[arc] = cost;
			//cout << "Arc: #" << i << " " << destNode << " " << origNode << " " << cost <<  endl;
		}
    }
    //static graph
	//g.build(nmax, arcList.begin(), arcList.end());

    //populate costMap
    //for (unsigned int i = 0; i < mmax; ++i)
    //    lengthVal[ g.arc(i) ] = pC[i];


	//init Dijkstra here
    dijk = new DijkstraInternal(g,lengthVal);
	//DijkstraInternal dijk(g,lengthVal);
}

void SPT_LEM_2Heap::SetOrigin( unsigned int NewOrg )
{
    //1. uint to Lemon Node
	orig = nodes[NewOrg-1];
	//multiple sources?
	//dijk->addSource(orig);
}

void SPT_LEM_2Heap::SetDest( unsigned int NewDst )
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
bool SPT_LEM_2Heap::Reached( unsigned int NodeID )
{
    //1. uint to Lemon Node
	SmartDigraph::Node node = nodes[NodeID-1];
	//StaticDigraph::Node node = nodes [NodeID-1];
    return dijk->reached(node);
}

void SPT_LEM_2Heap::GetPath ( unsigned int Dst, unsigned int *outSn, unsigned int *outEn )
{
	if (!allDests)
	{
		//id to lemon node
		SmartDigraph::Node pathDest = nodes[Dst-1];
		Path<SmartDigraph> path = dijk->path(pathDest);
		/*StaticDigraph::Node pathDest = nodes[Dst-1];
		Path<StaticDigraph> path = dijk->path(pathDest);*/

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

unsigned int* SPT_LEM_2Heap::ArcPredecessors( void )
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

unsigned int* SPT_LEM_2Heap::Predecessors( void )
{
	predecessors = new unsigned int [MCFnmax()+1];
	predecessors[0] = 0; //first entry of pred has no predecessor
    for (int i = 1; i < MCFnmax()+1; i++)
    {
        predecessors[i] = g.id(dijk->predNode(nodes[i-1])) + 1;
    }
	return predecessors;
}

void SPT_LEM_2Heap::GetArcPredecessors( unsigned int *outArcPrd )
{
	unsigned int size = MCFnmax()+1;
	auto arcPredecessors = ArcPredecessors();
	memcpy(outArcPrd, arcPredecessors, size * sizeof (unsigned int ) );
	delete[] arcPredecessors;
}

void SPT_LEM_2Heap::GetPredecessors( unsigned int *outPrd )
{
	unsigned int size = MCFnmax()+1;
	auto predecessors = Predecessors();
	memcpy(outPrd, predecessors, size * sizeof (unsigned int ) );
	delete[] predecessors;
}

unsigned int SPT_LEM_2Heap::MCFnmax( void)
{
	return ( this->nmax );
}

unsigned int SPT_LEM_2Heap::MCFmmax( void)
{
	return ( this->mmax );
}

void SPT_LEM_2Heap::PrintResult( void )
{
//    SmartDigraph::NodeMap<double> d = dijk->distMap();
    //depointerize distMap

    SmartDigraph::NodeMap<double>& distMapVal = *distMap;
    //StaticDigraph::NodeMap<double>& distMapVal = *distMap;
	cout << "***********************************************" << endl;
	cout << "C++ Output START" << endl;
	if (!allDests)
	{
		cout << "Dest reached: " << dijk->reached(dest) << endl;
	}
    cout << "Orig reached: " << dijk->reached(orig) << endl;
	if (!allDests)
	{
		cout << "Path cost to dest: " << distMapVal[dest] << endl;
	}
	cout << "C++ Output END" << endl;
	cout << "***********************************************" << endl;
    //cout << "Path cost to orig: " << distMapVal[orig] << endl;
    //printf("%i\n", distMapVal[start]);
}

SPT_LEM_2Heap::~SPT_LEM_2Heap()
{
    //dtor
    delete dijk;
	delete distMap;
	delete length;
}
