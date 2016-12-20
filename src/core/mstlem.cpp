#include "mstlem.h"

using namespace netxpert::core;

MST_LEMON::MST_LEMON()
{
    //ctor
}

MST_LEMON::~MST_LEMON()
{
	//TEST
	if (costMapPtr)
        delete costMapPtr;
    if (edgeBoolMapPtr)
        delete edgeBoolMapPtr;
}

void MST_LEMON::SolveMST(void)
{
	edgeBoolMapPtr = new SmartDigraph::ArcMap<bool>(g);

	//depointerize
	SmartDigraph::ArcMap<double>& costMap = *costMapPtr;
	SmartDigraph::ArcMap<bool>& edgeBoolMap = *edgeBoolMapPtr;

	totalCost = kruskal(g, costMap, edgeBoolMap);

}

void MST_LEMON::LoadNet( unsigned int nmx , unsigned int mmx , unsigned int pn , unsigned int pm ,
		      double *pU , double *pC , double *pDfct ,
		      unsigned int *pSn , unsigned int *pEn )
{
    //cout << "Entering LoadNet().." << endl;
	nmax = nmx;
	mmax = mmx;

    costMapPtr = new SmartDigraph::ArcMap<double> (g);

	nodes.resize(nmax);
    for (unsigned int i = 0; i < nmax; ++i) {
      nodes[i] = g.addNode();
	}
    // construct arcs
    //depointerize
	SmartDigraph::ArcMap<double>& costMap = *costMapPtr;
	double cost;
    SmartDigraph::Arc arc;
	unsigned int source;
	unsigned int target;

    /*cout << "nmx: " << nmx << endl;
    cout << "mmx: " << mmx << endl;*/

	for (unsigned int i = 0; i < mmx; ++i) {
		source = pSn[i];
		target = pEn[i];
		cost = pC[i];
		arc = g.addArc(nodes[source-1],nodes[target-1]);
		costMap[arc] = cost;
		//cout << "Arc: #" << i << " " << source << " " << target << " " << cost <<  endl;
	}
	//cout << "ready loading arcs" << endl;
}


unsigned int MST_LEMON::MCFnmax( void)
{
	return ( nmax );
}

unsigned int MST_LEMON::MCFmmax( void)
{
	return ( mmax );
}

void MST_LEMON::GetMST (unsigned int* outStartNodes, unsigned int* outEndNodes)
{
	//depointerize
	SmartDigraph::ArcMap<bool>& edgeBoolMap = *edgeBoolMapPtr;
	unsigned int source;
	unsigned int target;
	unsigned int size = MCFmmax();
	int counter = 0;
	unsigned int* testOutSnds = new unsigned int [MCFmmax()];
	unsigned int* testOutEnds = new unsigned int [MCFmmax()];
	for(SmartDigraph::ArcIt it(g); it!=INVALID; ++it)
	{
		//If Arc is a part of the MST the following is true
		if (edgeBoolMap[it]) {
			SmartDigraph::Arc a = it;
			source = g.id(g.source(a)) + 1;
			target = g.id(g.target(a)) + 1;
		}
		else
		{
			source = UINT_MAX;
			target = UINT_MAX;
		}
		//outStartNodes[counter] = source;
		//outEndNodes[counter] = target;
		testOutSnds[counter] = source;
		testOutEnds[counter] = target;
		counter = counter + 1;
	}
	memcpy(outStartNodes, testOutSnds, size * sizeof (unsigned int));
	memcpy(outEndNodes, testOutEnds, size * sizeof (unsigned int));

	delete[] testOutSnds;
	delete[] testOutEnds;
}

double MST_LEMON::MSTGetF0()
{
	return totalCost;
}

