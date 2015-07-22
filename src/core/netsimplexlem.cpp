#include "netsimplexlem.h"

using namespace std;
using namespace netxpert;

NetworkSimplex::NetworkSimplex( void )
{

}

void NetworkSimplex::SolveMCF( void )
{
	//output
	flowPtr = new lemon::SmartDigraph::ArcMap<double>(g);
	//lemon::SmartDigraph::ArcMap<double>& flowMap = *flowPtr;

	//output
	//nsimplexPtr->flowMap(flowMap);
	status = nsimplexPtr->run();
	//cout << type << endl;
}

void NetworkSimplex::LoadNet( unsigned int nmx , unsigned int mmx , unsigned int pn , unsigned int pm ,
		      double *pU , double *pC , double *pDfct ,
		      unsigned int *pSn , unsigned int *pEn )
{
	nmax = nmx;
	mmax = mmx;
	unsigned int source;
	unsigned int target;

    costsPtr = new lemon::SmartDigraph::ArcMap<double> (g);
	capacityPtr = new lemon::SmartDigraph::ArcMap<double> (g);
	supplyPtr = new lemon::SmartDigraph::NodeMap<double> (g);

	//depointerize
	lemon::SmartDigraph::ArcMap<double>& costMap = *costsPtr;
	lemon::SmartDigraph::ArcMap<double>& capacityMap = *capacityPtr;
	lemon::SmartDigraph::NodeMap<double>& supplyMap = *supplyPtr;

	//Populate all nodes
    //std::vector<typename SmartDigraph::Node> nodes;
    nodes.resize(nmax);
    for (unsigned int i = 0; i < nmax; ++i) {
		nodes[i] = g.addNode();
		supplyMap.set(nodes[i], pDfct[i]);
    }

    // construct arcs
	double cost;
	double capacity;

    lemon::SmartDigraph::Arc arc;

	for (unsigned int i = 0; i < mmx; ++i) {
		source = pSn[i];
		target = pEn[i];
		cost = pC[i];
		capacity = pU[i];
		arc = g.addArc(nodes[source-1],nodes[target-1]);
		capacityMap.set(arc, capacity);
		costMap.set(arc, cost);
		//cout << "Arc: #" << i << " " << origNode << " " << destNode << " " << cost <<  endl;
	}
	//init
	nsimplexPtr = new lemon::NetworkSimplex<lemon::SmartDigraph,int,double>(g, false);
	nsimplexPtr->upperMap(capacityMap);
	nsimplexPtr->costMap(costMap);
	nsimplexPtr->supplyMap(supplyMap);
}


unsigned int NetworkSimplex::MCFnmax( void)
{
	return ( nmax );
}

unsigned int NetworkSimplex::MCFmmax( void)
{
	return ( mmax );
}

double NetworkSimplex::MCFGetFO( void )
{
	return nsimplexPtr->totalCost();
}

void NetworkSimplex::MCFGetX( double* flow )
{
	lemon::SmartDigraph::ArcMap<double>& flowMap = *flowPtr;
	nsimplexPtr->flowMap(flowMap);
	double* flowX = new double[MCFmmax()];
	for (int i = 0; i < MCFmmax(); i++)
	{
		flowX[i] = flowMap[g.arcFromId(i)];
		//cout << flowMap[g.arcFromId(i)] << endl;

		//cout << flowMap[i] << endl;
		//flow[i] = nsimplexPtr->flow(g.arcFromId(i));
	}
	memcpy(flow, flowX, MCFmmax() * sizeof (double) );
	delete[] flowX;
}

void NetworkSimplex::MCFArcs( unsigned int* startNodes, unsigned int* endNodes)
{
	for (int i = 0; i < MCFmmax(); i++)
	{
		startNodes[i] = g.id(g.source(g.arcFromId(i))) + 1;
		endNodes[i] = g.id(g.target(g.arcFromId(i))) + 1;
	}
}

void NetworkSimplex::MCFCosts( double* costs )
{
	lemon::SmartDigraph::ArcMap<double>& costMap = *costsPtr;
	for (int i = 0; i < MCFmmax(); i++)
	{
		costs[i] = costMap[g.arcFromId(i)];
	}
}

void NetworkSimplex::PrintResult( void )
{
 //   SmartDigraph::NodeMap<double>& distMapVal = *distMap;
	//cout << "***********************************************" << endl;
	//cout << "C++ Output START" << endl;
	//if (!allDests)
	//{
	//	cout << "Dest reached: " << dijk->reached(dest) << endl;
	//}
 //   cout << "Orig reached: " << dijk->reached(orig) << endl;
	//if (!allDests)
	//{
	//	cout << "Path cost to dest: " << distMapVal[dest] << endl;
	//}
	//cout << "C++ Output END" << endl;
	//cout << "***********************************************" << endl;
    //cout << "Path cost to orig: " << distMapVal[orig] << endl;
    //printf("%i\n", distMapVal[start]);
}

int NetworkSimplex::MCFGetStatus()
{
	/* LEMON

	   INFEASIBLE = 0
	   OPTIMAL = 1
	   UNBOUNDED = 2
	  */
	switch (status)
	{
	case lemon::NetworkSimplex<lemon::SmartDigraph, int, double>::INFEASIBLE:
		return 2;
		break;
	case lemon::NetworkSimplex<lemon::SmartDigraph, int, double>::OPTIMAL:
		return 0;
		break;
	case lemon::NetworkSimplex<lemon::SmartDigraph, int, double>::UNBOUNDED:
		return 3;
		break;
	default:
		return -1;
		break;
	}
	/* MCFClass
		kUnSolved = -1 , ///< no solution available
        kOK = 0 ,        ///< optimal solution found

        kStopped ,       ///< optimization stopped
        kUnfeasible ,    ///< problem is unfeasible
        kUnbounded ,     ///< problem is unbounded
        kError
	*/

}

void NetworkSimplex::CheckPSol()
{

}

void NetworkSimplex::CheckDSol()
{

}
NetworkSimplex::~NetworkSimplex(void)
{
	delete nsimplexPtr;
	delete costsPtr;
	delete capacityPtr;
	delete supplyPtr;
	delete flowPtr;
}

