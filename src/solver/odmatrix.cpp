#include "odmatrix.h"

using namespace netxpert;
using namespace std;

OriginDestinationMatrix::OriginDestinationMatrix(Config& cnfg)
{
    //ctor
    LOGGER::LogInfo("OriginDestinationMatrix Solver instantiated");
    algorithm = cnfg.SptAlgorithm;
    isDirected = cnfg.IsDirected;
    sptHeapCard = cnfg.SPTHeapCard;
    geometryHandling = cnfg.GeometryHandling;
}

OriginDestinationMatrix::~OriginDestinationMatrix()
{
    //dtor
}

void OriginDestinationMatrix::Solve(string net){
    throw;
}

void OriginDestinationMatrix::Solve(Network& net)
{
    this->net = &net;

    unsigned int arcCount = net.GetCurrentArcCount();
    unsigned int nodeCount = net.GetCurrentNodeCount();

    if (destinationNodes.size() == 0 || originNodes.size() == 0)
    {
        LOGGER::LogFatal("Origin Nodes or Destination nodes in ODMatrix Solver must not be null!");
        throw;
    }
    else if (destinationNodes.size() > 0 && originNodes.size() > 0)
    {
        //set the size for the heap to m/n (arcs/nodes) if -1
        checkSPTHeapCard(arcCount, nodeCount);
        solve(net, originNodes, destinationNodes, isDirected);
    }
}
void OriginDestinationMatrix::checkSPTHeapCard(unsigned int arcCount, unsigned int nodeCount)
{
    if (sptHeapCard == -1)
    {
        switch (isDirected)
        {
            case true:
                sptHeapCard = (int) (arcCount / nodeCount) ;
                break;
            case false:
                sptHeapCard = (int) (arcCount * 2 / nodeCount );
                break;
        }
    }
}


/**
* n - n
*/
void OriginDestinationMatrix::solve (Network& net, vector<unsigned int>& origs,
                                vector<unsigned int>& dests, bool isDirected)
{
    //vector<long> ign = { 0, -1, (long)4294967295, (long)4261281277 };
    // 0 is a valid ignore value for linux!
    //TODO check in windows
    vector<long> ign  { -1, (long)4294967295, (long)4261281277 };

    unsigned int nmax;
    unsigned int anz;
    /*vector<unsigned int> a_pre;
    vector<unsigned int> pre;
    vector<unsigned int> nodes;*/

    vector<unsigned int> sNds;
    vector<unsigned int> eNds;
    vector<double> supply;
    vector<double> costs;
    vector<double> caps;

    if (!validateNetworkData( net, origs, dests ))
        //throw new InvalidValueException(string.Format("Problem data does not fit the {0} Solver!", this.ToString()));
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetMaxArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetMaxNodeCount() ));
    LOGGER::LogDebug("Origs: "+ to_string(origs.size()));
    LOGGER::LogDebug("Dests: "+ to_string(dests.size()));
    LOGGER::LogDebug("Solving..");

    //Read the network
    convertInternalNetworkToSolverData(net, sNds, eNds, supply, caps, costs);

    double totalCost = 0;

    //Main loop for calculating the ODMatrix
    int counter = 0;
    auto origsSize = origs.size();

    //for (unsigned int orig : origs)
    /* OpenMP only wants iterator style in for-loops - no auto for loops
       The loop for the ODMatrix Calculation is being computed paralell with the critical keyword.
    */

    //#pragma omp parallel for shared(origs)
    //make spt local to be copied for parallel proc
    //no class member variables can be parallized
	#pragma omp parallel default(shared) num_threads(LOCAL_NUM_THREADS)
    {
    shared_ptr<ISPTree> lspt;
    switch (algorithm)
    {
        case SPTAlgorithm::Dijkstra_MCFClass:
            lspt = shared_ptr<ISPTree>(new SPTree_Dijkstra(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
            break;
        case SPTAlgorithm::LQueue_MCFClass:
            lspt = shared_ptr<ISPTree>(new SPTree_LQueue(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
            break;
        case SPTAlgorithm::LDeque_MCFClass:
            lspt = shared_ptr<ISPTree>(new SPTree_LDeque(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
            break;
        case SPTAlgorithm::Dijkstra_Heap_MCFClass:
            lspt = shared_ptr<ISPTree>(new SPTree_Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected, sptHeapCard));
            break;
        case SPTAlgorithm::Dijkstra_2Heap_LEMON:
            lspt = shared_ptr<ISPTree>(new SPT_LEM_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
            break;
        default:
            lspt = shared_ptr<ISPTree>(new SPT_LEM_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
            break;
    }

    lspt->LoadNet( static_cast<unsigned int>( net.GetMaxNodeCount() ), static_cast<unsigned int>( net.GetMaxArcCount()),
                        static_cast<unsigned int>( net.GetMaxNodeCount() ),static_cast<unsigned int>( net.GetMaxArcCount() ),
                        caps.data(), costs.data(), supply.data(), sNds.data(), eNds.data());

    nmax = lspt->MCFnmax();
    anz = lspt->MCFnmax();

    vector<unsigned int> a_pre;
    vector<unsigned int> pre;
    vector<unsigned int> nodes;

    a_pre.resize(anz + 1);
    pre.resize(anz + 1);
    nodes.resize(anz + 1);

    vector<unsigned int>::iterator origIt;

    for (origIt = origs.begin(); origIt!=origs.end(); origIt++)
    {
        #pragma omp single nowait
        {
        unsigned int orig = *origIt;
        unordered_set< unsigned int > intReachedDests;

        lspt->SetOrigin(orig);

        int routesLeft = origsSize - counter;
        LOGGER::LogDebug("# "+ to_string(omp_get_thread_num())  +": Calculating routes from " + net.GetOriginalStartOrEndNodeID(orig) + ", # "+
                            to_string(routesLeft) +" left..");
        // Set dests to UINT_MAX --> no dest setting at all!
        lspt->SetDest(UINT_MAX);
        //LOGGER::LogDebug("Starting to solve SPT..");
        lspt->ShortestPathTree();
        //LOGGER::LogDebug("SPT solved! ");

        lspt->GetArcPredecessors(a_pre.data());
        lspt->GetPredecessors(pre.data());

        //omp: local intReachedDests --> no concurrent writes per thread
        for (auto dest : dests)
        {
            if (lspt->Reached(dest))
                intReachedDests.insert(dest);
        }

        unordered_map<unsigned int,unsigned int> arcs;
        int i;
        //omp: local arcs --> no concurrent writes per thread
        for (i = nmax; i > 0; i--)
        {
            if (a_pre[i] != ign[0] && a_pre[i] != ign[1])
                arcs.insert( make_pair(i,pre[i]) );
        }

        // Direction is irrelevant - the solver deals with the direction.
        // Thus it's not necessary to pay attention at this when building the route
        // out of the predecessors.

        // Get all routes from orig to dest in nodes-List

        //for (unsigned int dest : dests)
        //{

        vector<unsigned int> route;
        vector<unsigned int>::iterator destIt;
        for (destIt=dests.begin(); destIt!=dests.end(); destIt++)
        {
            auto dest = *destIt;
            bool isDestReached;
            if ( intReachedDests.count(dest) > 0 )
                isDestReached = true;
            else
                isDestReached = false;

            if (orig != dest && isDestReached)
            {
                //route is reference
                const double costPerRoute = buildCompressedRoute(route, orig, dest, arcs);
                totalCost += costPerRoute;

                //Neuer vector muss sein, wegen clear() Methode weiter unten - sonst werden
                // bei sps auch die Vektoren geleert.
                #pragma omp critical
                {
                shortestPaths.insert( make_pair( ODPair {orig, dest},
                                      make_pair( route, costPerRoute ) ));//vector<unsigned int> (route),
                                                 //   costPerRoute)) );
                odMatrix.insert( make_pair( ODPair {orig, dest},
                                    costPerRoute));
                route.clear();
                reachedDests.push_back(dest);
                }

            }
            if (!isDestReached)
            {
                //LOGGER::LogError("Destination "+ net.GetOriginalStartOrEndNodeID(dest) +" unreachable!");
            }
        }
        optimum = totalCost; //totalCost wird immer weiter aufsummiert für jedes neues OD-Paar
        counter += 1;

    }//omp single
    }
    }//omp parallel
}

SPTAlgorithm OriginDestinationMatrix::GetAlgorithm() const
{
    return this->algorithm;
}
void OriginDestinationMatrix::SetAlgorithm(SPTAlgorithm sptAlgorithm)
{
    this->algorithm = sptAlgorithm;
}

GEOMETRY_HANDLING OriginDestinationMatrix::GetGeometryHandling() const
{
    return this->geometryHandling;
}
void OriginDestinationMatrix::SetGeometryHandling(GEOMETRY_HANDLING geomHandling)
{
    this->geometryHandling = geomHandling;
}

int OriginDestinationMatrix::GetSPTHeapCard() const
{
    return this->sptHeapCard;
}
void OriginDestinationMatrix::SetSPTHeapCard(int heapCard)
{
    this->sptHeapCard = heapCard;
}

vector<unsigned int> OriginDestinationMatrix::GetOrigins() const
{
    return this->originNodes;
}
void OriginDestinationMatrix::SetOrigins(vector<unsigned int>& origs)
{
    this->originNodes = origs;
}
void OriginDestinationMatrix::SetOrigins(vector<pair<unsigned int, string>>& origs)
{
    vector<unsigned int> newOrigs;
    for (auto s : origs)
        newOrigs.push_back(s.first);
    this->originNodes = newOrigs;
}

vector<unsigned int> OriginDestinationMatrix::GetDestinations() const
{
    return this->destinationNodes;
}
void OriginDestinationMatrix::SetDestinations(vector<unsigned int>& dests)
{
    this->destinationNodes = dests;
}

void OriginDestinationMatrix::SetDestinations(vector<pair<unsigned int, string>>& dests)
{
    vector<unsigned int> newDests;
    for (auto e : dests)
        newDests.push_back(e.first);
    this->destinationNodes = newDests;
}
vector<unsigned int> OriginDestinationMatrix::GetReachedDests() const
{
    return this->reachedDests;
}
unordered_map<ODPair, CompressedPath> OriginDestinationMatrix::GetShortestPaths() const
{
    return this->shortestPaths;
}
unordered_map<ODPair, double> OriginDestinationMatrix::GetODMatrix() const
{
    return this->odMatrix;
}

double OriginDestinationMatrix::GetOptimum() const {
    return this->optimum;
}

vector<InternalArc> OriginDestinationMatrix::UncompressRoute(unsigned int orig, vector<unsigned int>& ends) const
{
    vector<InternalArc> startsNends;
    for (int i = 0; i < ends.size(); i++)
    {
        if (i != ends.size() - 1)
            startsNends.push_back( InternalArc { ends[i + 1], ends[i] } );
        else
            startsNends.push_back( InternalArc {  orig, ends[i] } );
    }
    return startsNends;
}

void OriginDestinationMatrix::convertInternalNetworkToSolverData(Network& net, vector<unsigned int>& sNds,
            vector<unsigned int>& eNds, vector<double>& supply, vector<double>& caps, vector<double>& costs)
{
    // Die Größe der Arrays müssen passen (ob 0 drin steht ist egal, sonst gibts später bei Dispose
    // das böse Erwachen in Form eines Heap Corruption errors bzw. einer System Access Violation

    Arcs arcs = net.GetInternalArcData();
    vector<InternalArc> keys;
    for(Arcs::iterator it = arcs.begin(); it != arcs.end(); ++it) {
      keys.push_back(it->first);
    }

    // Für Shortest Path Tree Tree auf die Richtung achten
    // --> doppelter Input der Kanten notwendig bei undirected
    sNds.resize(keys.size());
    eNds.resize(keys.size());
    //cout << "size of arcs: " << keys.size() << endl;
    for (int i = 0; i < keys.size(); i++)
    {
        sNds[i] = keys[i].fromNode;
        eNds[i] = keys[i].toNode;
    }

    costs.resize(keys.size() ); //Größe muss passen!
    caps.resize(keys.size(), 0);  //Größe muss passen!
    for (int i = 0; i < keys.size(); i++)
    {
        ArcData oldArcData;
        if (arcs.count(keys[i]) > 0)
            oldArcData = arcs.at(keys[i]);
        costs[i] = oldArcData.cost;
        //SPTree does not care about capacity
        //caps[i] = 0; //oldArcData.capacity;
    }

    supply.resize( net.GetMaxNodeCount(), 0 ); //Größe muss passen!
    //cout << "supply vector size: "<< supply.size() << endl;
    /*cout << "net supply size: " << net.GetNodeSupplies().size() <<endl;
    for (auto item : net.GetNodeSupplies() )
    {
        unsigned int key = item.first;
        NodeSupply value = item.second;
        // key is 1-based thus -1 for index
        // only care for real supply and demand values
        // transshipment nodes (=0) are already present in the array (because of new array)
        supply[key - 1] = value.supply;
    }*/
    //cout << "ready converting data" << endl;
}

bool OriginDestinationMatrix::validateNetworkData(Network& net, vector<unsigned int>& origs, vector<unsigned int>& dests)
{
    bool valid = false;

    //IMPORTANT Checks
    for (unsigned int orig : origs)
    {
        if (orig == 0){
            LOGGER::LogFatal("Origin Node ID must be greater than zero!");
            throw;
            //throw new InvalidValueException("Origin Node ID must be greater than zero!");
        }
        if (orig > net.GetMaxNodeCount() ) {
            //throw new InvalidValueException("Origin Node ID must not exceed net.MaxNodeCount!");
            LOGGER::LogFatal("Origin Node ID must not exceed maximum node count of network!");
            throw;
        }
    }

    for (auto dest : dests)
    {
        if (dest > net.GetMaxNodeCount() ) {
            //throw new InvalidValueException("Destination Node ID must not exceed net.MaxNodeCount!");
            LOGGER::LogFatal("Destination Node ID must not exceed maximum node count of network!");
            throw;
        }
    }

    valid = true;
    return valid;
}

double OriginDestinationMatrix::buildCompressedRoute(vector<unsigned int>& route, unsigned int orig, unsigned int dest,
                                                unordered_map<unsigned int, unsigned int>& arcPredescessors)
{
    double totalCost = 0;
    //Neu - nur die Enden hinzufuegen
    unsigned int curr = dest;

    while (curr != orig)
    {
        route.push_back(curr);
        const InternalArc& ftNode  { arcPredescessors.at(curr), curr };

        //#pragma omp parallel shared(ftNode)
        //{
        //#pragma omp parallel reduction(+:totalCost)
        //{
        totalCost += getArcCost( ftNode );
        //}
        curr = arcPredescessors.at(curr);
    }

    return totalCost;
}
double OriginDestinationMatrix::getArcCost(const InternalArc& arc)
{
    if (isDirected)
    {
        //TODO: Suche von newArcs über oldArcs zu intArcs?
        // Idee: von den kleinsten listen zu den größten
        const Arcs& intArcs = net->GetInternalArcData();
        if (intArcs.count( arc ) == 0)
        {
            const Arcs& oldArcs = net->GetOldArcs();
            if (oldArcs.count(arc) == 0)
            {
                const NewArcs& newArcs = net->GetNewArcs();
                const NewArc& outArcNew = newArcs.at(arc);
                return outArcNew.cost;
            }
            else {
                const ArcData& outArc = oldArcs.at(arc);
                return outArc.cost;
            }
        }
        else {
            const ArcData& outArc = intArcs.at(arc);
            return outArc.cost;
        }
    }
    else //Undirected
    {
        const Arcs& intArcs = net->GetInternalArcData();
        if (intArcs.count( arc ) == 0)
        {
            //reverse
            if (intArcs.count( InternalArc {arc.toNode, arc.fromNode} ) == 0)
            {
                const Arcs& oldArcs = net->GetOldArcs();
                if (oldArcs.count(arc) == 0)
                {
                    //reverse
                    if(oldArcs.count( InternalArc {arc.toNode, arc.fromNode} ) == 0)
                    {
                        const NewArcs& newArcs = net->GetNewArcs();
                        if(newArcs.count(arc) == 0)
                        {
                            //reverse
                            const NewArc& outArcNew = newArcs.at( InternalArc {arc.toNode, arc.fromNode} );
                            return outArcNew.cost;
                        }
                        else {
                            const NewArc& outArcNew = newArcs.at( arc );
                            return outArcNew.cost;
                        }
                    }
                    else {
                        const ArcData& outArc = oldArcs.at( InternalArc {arc.toNode, arc.fromNode} );
                        return outArc.cost;
                    }
                }
                else {
                    const ArcData& outArc = oldArcs.at( arc );
                    return outArc.cost;
                }
            }
            else {
                const ArcData& outArc = intArcs.at( InternalArc {arc.toNode, arc.fromNode} );
                return outArc.cost;
            }
        }
        else {
            const ArcData& outArc = intArcs.at( arc );
            return outArc.cost;
        }
    }
}
