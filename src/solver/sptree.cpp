#include "sptree.h"

using namespace std;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::data;
using namespace netxpert::io;
using namespace netxpert::core;
using namespace netxpert::utils;

ShortestPathTree::ShortestPathTree(Config& cnfg)
{
    //ctor
    LOGGER::LogInfo("ShortestPathTree Solver instantiated");
    algorithm = cnfg.SptAlgorithm;
    isDirected = cnfg.IsDirected;
    sptHeapCard = cnfg.SPTHeapCard;
    geometryHandling = cnfg.GeometryHandling;
    this->NETXPERT_CNFG = cnfg;
}

void ShortestPathTree::Solve(string net){
    throw;
}

void ShortestPathTree::Solve(Network& net)
{
    this->net = &net;

    unsigned int arcCount = net.GetCurrentArcCount();
    unsigned int nodeCount = net.GetCurrentNodeCount();

    //TODO: check for node supply in network
    /*NodeSupplies ns = net.GetNodeSupplies();
    for (auto kv : ns)
    {
        IntNodeID id = kv.first;
        NodeSupply s = kv.second;
        if (s.supply < 0)
            this->originNode = id;
        if (s.supply > 0)
            this->destinationNodes.push_back(id);
    }*/

    if (destinationNodes.size() == 0)
    {
        //set the size for the heap to m/n (arcs/nodes) if -1
        checkSPTHeapCard(arcCount, nodeCount);
        solve(net, originNode, isDirected);
    }
    else if (destinationNodes.size() == 1)
    {
        //set the size for the heap to m/n (arcs/nodes) if -1
        checkSPTHeapCard(arcCount, nodeCount);
        unsigned int dest = destinationNodes[0];
        solve(net, originNode, dest, isDirected);
    }
    else if (destinationNodes.size() > 1)
    {
        //set the size for the heap to m/n (arcs/nodes) if -1
        checkSPTHeapCard(arcCount, nodeCount);
        solve(net, originNode, destinationNodes, isDirected);
    }
}
void ShortestPathTree::checkSPTHeapCard(unsigned int arcCount, unsigned int nodeCount)
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
* 1 - all
*/
void ShortestPathTree::solve (Network& net, unsigned int orig, bool isDirected)
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

    try
    {
        switch (algorithm)
        {
            case SPTAlgorithm::Dijkstra_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_Dijkstra(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                            isDirected));
                break;
            case SPTAlgorithm::LQueue_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_LQueue(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                            isDirected));
                break;
            case SPTAlgorithm::LDeque_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_LDeque(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                            isDirected));
                break;
            case SPTAlgorithm::Dijkstra_Heap_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                            isDirected, sptHeapCard));
                break;
            case SPTAlgorithm::Dijkstra_2Heap_LEMON:
                spt = unique_ptr<ISPTree>(new SPT_LEM_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                            isDirected));
                break;
            case SPTAlgorithm::Bijkstra_2Heap_LEMON:
                spt = unique_ptr<ISPTree>(new SPT_LEM_Bijkstra_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
                break;
            case SPTAlgorithm::Dijkstra_dheap_BOOST:
                spt = unique_ptr<ISPTree>(new SPT_BGL_Dijkstra(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
                break;
            default:
                break;
        }
    }
    catch (exception& ex)
    {
        /*if (ex.GetInnermostException() is DllNotFoundException)
        {
            DllNotFoundException dllEx = (DllNotFoundException)ex.GetInnermostException();
            Logger.WriteLog(string.Format("Fehler beim Instanziieren des MST-Solvers (Kern): {0}", dllEx.Message), LogLevel.Fatal);
            throw dllEx;
        }
        else
        {
            Logger.WriteLog(string.Format("Fehler beim Instanziieren des MST-Solvers (Kern): {0}", ex.InnerException), LogLevel.Fatal);
            Logger.WriteLog(string.Format("StackTrace: {0}", ex.StackTrace), LogLevel.Fatal);
            throw ex;
        }*/
    }

    if (!validateNetworkData( net, orig ))
        //throw new InvalidValueException(string.Format("Problem data does not fit the {0} Solver!", this.ToString()));
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetMaxArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetMaxNodeCount() ));
    LOGGER::LogDebug("Dests: "+ to_string(net.GetMaxNodeCount() ));
    LOGGER::LogDebug("Solving..");

    //Read the network
    try
    {
        convertInternalNetworkToSolverData(net, sNds, eNds, supply, caps, costs);

        //vector::data() returns direct pointer to data
        spt->LoadNet( static_cast<unsigned int>( net.GetMaxNodeCount() ), static_cast<unsigned int>( net.GetMaxArcCount()),
                        static_cast<unsigned int>( net.GetMaxNodeCount() ),static_cast<unsigned int>( net.GetMaxArcCount() ),
                        caps.data(), costs.data(), supply.data(), sNds.data(), eNds.data());
    }
    catch (exception& ex)
    {
        LOGGER::LogFatal( ex.what() );
    }

    double totalCost = 0;
    spt->SetOrigin(orig);

    try
    {
        LOGGER::LogDebug("Calculating routes from " + net.GetOriginalStartOrEndNodeID(orig) + " to all destinations in network..");
        // Solver fails on UNIT_MAX --> no dest setting at all!
        spt->SetDest( UINT_MAX );
        LOGGER::LogDebug("Starting to solve SPT..");
        spt->ShortestPathTree();
        LOGGER::LogDebug("SPT solved!");
    }
    catch (exception& ex)
    {
        //spt.Dispose();
        //throw new ApplicationException(ex.Message, ex.InnerException);
    }

    nmax = spt->MCFnmax();
    anz = spt->MCFnmax();

    //vector<unsigned int> a_pre;
    vector<unsigned int> pre;
    vector<unsigned int> nodes;

    //a_pre.reserve(anz + 1);
    pre.reserve(anz + 1);
    nodes.reserve(anz + 1);

    try
    {
        // vector::data() returns pointer
        //spt->GetArcPredecessors(a_pre.data());
        spt->GetPredecessors(pre.data());
    }
    catch (exception& ex)
    {
        //spt.Dispose();
        //throw new ApplicationException(ex.Message, ex.InnerException);
    }

    unordered_map<unsigned int,unsigned int> arcs;
    arcs.reserve(spt->MCFmmax());
    #pragma omp critical
    {
    for (int i = nmax; i > 0; i--)
    {
        //if (a_pre[i] != ign[0] && a_pre[i] != ign[1])
        if (pre[i] != ign[0] && pre[i] != ign[1])
            arcs.insert( make_pair( i,pre[i] ));
    }

    for (int i = 1; i < anz + 1; i++) {
        nodes.push_back( (unsigned int)i );
    }
    }//omp critical

    // Direction is irrelevant - the solver deals with the direction.
    // Thus it's not necessary to pay attention at this when building the route
    // out of the predecessors.
    reachedDests.clear();
    shortestPaths.clear();
    vector<unsigned int> route;

    #pragma omp critical
    {
    // Get all routes from orig to dest in nodes-List
    for (unsigned int dest : nodes)
    {
        //LOGGER::LogDebug(to_string(dest) + " of " + to_string(nodes.size()) );
        if (orig != dest && spt->Reached(dest))
        {
            //route is reference
            const double costPerRoute = buildCompressedRoute(route, orig, dest, arcs);
            totalCost = totalCost + costPerRoute;

            //Neuer vector muss sein, wegen clear() Methode weiter unten - sonst werden
            // bei sps auch die Vektoren geleert.
            shortestPaths.insert( make_pair( ODPair {orig, dest},
                                  make_pair( route, //vector<unsigned int> (route),
                                                costPerRoute)) );
            route.clear();
            reachedDests.push_back(dest);
        }
    }
    }//omp critical
    optimum = totalCost;
    //spt.Dispose();
}
/**
* 1 - 1
*/
void ShortestPathTree::solve (Network& net, unsigned int orig,
                                unsigned int dest, bool isDirected)
{
    //vector<long> ign = { 0, -1, (long)4294967295, (long)4261281277 };
    // 0 is a valid ignore value for linux!
    //TODO check in windows
    vector<long> ign  { -1, (long)4294967295, (long)4261281277 };

    unsigned int nmax;
    unsigned int anz;
    /*
    vector<unsigned int> a_pre;
    vector<unsigned int> pre;
    vector<unsigned int> nodes;*/

    vector<unsigned int> sNds;
    vector<unsigned int> eNds;
    vector<double> supply;
    vector<double> costs;
    vector<double> caps;

    try
    {
        switch (algorithm)
        {
            case SPTAlgorithm::Dijkstra_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_Dijkstra(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                            isDirected));
                break;
            case SPTAlgorithm::LQueue_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_LQueue(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                            isDirected));
                break;
            case SPTAlgorithm::LDeque_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_LDeque(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                            isDirected));
                break;
            case SPTAlgorithm::Dijkstra_Heap_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected, sptHeapCard));
                break;
            case SPTAlgorithm::Dijkstra_2Heap_LEMON:
                spt = unique_ptr<ISPTree>(new SPT_LEM_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
                break;
            case SPTAlgorithm::Bijkstra_2Heap_LEMON:
                {

                LOGGER::LogDebug("Bijkstra..");
                spt = unique_ptr<ISPTree>(new SPT_LEM_Bijkstra_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));

                }
                break;
            case SPTAlgorithm::Dijkstra_dheap_BOOST:
                spt = unique_ptr<ISPTree>(new SPT_BGL_Dijkstra(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
                break;
            default:
                break;
        }
    }
    catch (exception& ex)
    {
        /*if (ex.GetInnermostException() is DllNotFoundException)
        {
            DllNotFoundException dllEx = (DllNotFoundException)ex.GetInnermostException();
            Logger.WriteLog(string.Format("Fehler beim Instanziieren des MST-Solvers (Kern): {0}", dllEx.Message), LogLevel.Fatal);
            throw dllEx;
        }
        else
        {
            Logger.WriteLog(string.Format("Fehler beim Instanziieren des MST-Solvers (Kern): {0}", ex.InnerException), LogLevel.Fatal);
            Logger.WriteLog(string.Format("StackTrace: {0}", ex.StackTrace), LogLevel.Fatal);
            throw ex;
        }*/
    }

    if (!validateNetworkData( net, orig, dest ))
        //throw new InvalidValueException(string.Format("Problem data does not fit the {0} Solver!", this.ToString()));
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetMaxArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetMaxNodeCount() ));
    LOGGER::LogDebug("Solving..");

    //Read the network
    try
    {
        convertInternalNetworkToSolverData(net, sNds, eNds, supply, caps, costs);

        //vector::data() returns direct pointer to data
        spt->LoadNet( static_cast<unsigned int>( net.GetMaxNodeCount() ), static_cast<unsigned int>( net.GetMaxArcCount()),
                        static_cast<unsigned int>( net.GetMaxNodeCount() ),static_cast<unsigned int>( net.GetMaxArcCount() ),
                        caps.data(), costs.data(), supply.data(), sNds.data(), eNds.data());
    }
    catch (exception& ex)
    {
        LOGGER::LogFatal( ex.what() );
    }

    double totalCost = 0;
    spt->SetOrigin(orig);

    try
    {
        LOGGER::LogDebug("Calculating routes from " + net.GetOriginalStartOrEndNodeID(orig) + " to " +
                                net.GetOriginalStartOrEndNodeID(dest) );
        spt->SetDest(dest);
        LOGGER::LogDebug("Starting to solve SPT..");
        spt->ShortestPathTree();
        LOGGER::LogDebug("SPT solved!");
    }
    catch (exception& ex)
    {
        //spt.Dispose();
        //throw new ApplicationException(ex.Message, ex.InnerException);
    }
    bool isDestReached = spt->Reached(dest);

    if (!isDestReached)
    {
        LOGGER::LogError("Destination "+ net.GetOriginalStartOrEndNodeID(dest) +" unreachable!");
        //spt.Dispose();
    }

    nmax = spt->MCFnmax();
    anz = spt->MCFnmax();

    //vector<unsigned int> a_pre;
    vector<unsigned int> pre;
    vector<unsigned int> nodes;

    //a_pre.reserve(anz + 1);
    pre.reserve(anz + 1);
    nodes.reserve(anz + 1);

    try
    {
        // vector::data() returns pointer
        //spt->GetArcPredecessors(a_pre.data());
        spt->GetPredecessors(pre.data());
    }
    catch (exception& ex)
    {
        //spt.Dispose();
        //throw new ApplicationException(ex.Message, ex.InnerException);
    }

    unordered_map<unsigned int,unsigned int> arcs (nmax);
    arcs.reserve(spt->MCFmmax());
    for (unsigned int i = nmax; i > 0; i--)
    {
        //if (a_pre[i] != ign[0] && a_pre[i] != ign[1])
        if (pre[i] != ign[0] && pre[i] != ign[1])
            arcs.insert( make_pair( i,pre[i] ));
    }

    // Direction is irrelevant - the solver deals with the direction.
    // Thus it's not necessary to pay attention at this when building the route
    // out of the predecessors.
    reachedDests.clear();
    shortestPaths.clear();
    vector<unsigned int> route;
    //route is reference
    totalCost = buildCompressedRoute(route, orig, dest, arcs);

    //Neuer vector muss sein, wegen clear() Methode weiter unten - sonst werden
    // bei sps auch die Vektoren geleert.
    shortestPaths.insert( make_pair( ODPair {orig, dest},
                          make_pair( route, //vector<unsigned int> (route),
                                        totalCost)) );

    route.clear();
    reachedDests.push_back(dest);
    optimum = totalCost;
    //spt.Dispose();
}

/**
* 1 - n
*/
void ShortestPathTree::solve (Network& net, unsigned int orig,
                                vector<unsigned int> dests, bool isDirected)
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

    try
    {
        switch (algorithm)
        {
            case SPTAlgorithm::Dijkstra_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_Dijkstra(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
                break;
            case SPTAlgorithm::LQueue_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_LQueue(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                          isDirected));
                break;
            case SPTAlgorithm::LDeque_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_LDeque(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                    isDirected));
                break;
            case SPTAlgorithm::Dijkstra_Heap_MCFClass:
                spt = unique_ptr<ISPTree>(new SPTree_Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                    isDirected, sptHeapCard));
                break;
            case SPTAlgorithm::Dijkstra_2Heap_LEMON:
                spt = unique_ptr<ISPTree>(new SPT_LEM_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
                break;
            case SPTAlgorithm::Bijkstra_2Heap_LEMON:
                spt = unique_ptr<ISPTree>(new SPT_LEM_Bijkstra_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
                break;
            case SPTAlgorithm::Dijkstra_dheap_BOOST:
                spt = unique_ptr<ISPTree>(new SPT_BGL_Dijkstra(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
                break;
            default:
                break;
        }
    }
    catch (exception& ex)
    {
        /*if (ex.GetInnermostException() is DllNotFoundException)
        {
            DllNotFoundException dllEx = (DllNotFoundException)ex.GetInnermostException();
            Logger.WriteLog(string.Format("Fehler beim Instanziieren des MST-Solvers (Kern): {0}", dllEx.Message), LogLevel.Fatal);
            throw dllEx;
        }
        else
        {
            Logger.WriteLog(string.Format("Fehler beim Instanziieren des MST-Solvers (Kern): {0}", ex.InnerException), LogLevel.Fatal);
            Logger.WriteLog(string.Format("StackTrace: {0}", ex.StackTrace), LogLevel.Fatal);
            throw ex;
        }*/
    }

    if (!validateNetworkData( net, orig, dests ))
        //throw new InvalidValueException(string.Format("Problem data does not fit the {0} Solver!", this.ToString()));
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetMaxArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetMaxNodeCount() ));
    LOGGER::LogDebug("Dests: "+ to_string(dests.size()));
    LOGGER::LogDebug("Solving..");

    //Read the network
    try
    {
        convertInternalNetworkToSolverData(net, sNds, eNds, supply, caps, costs);

        //vector::data() returns direct pointer to data
        spt->LoadNet( static_cast<unsigned int>( net.GetMaxNodeCount() ), static_cast<unsigned int>( net.GetMaxArcCount()),
                        static_cast<unsigned int>( net.GetMaxNodeCount() ),static_cast<unsigned int>( net.GetMaxArcCount() ),
                        caps.data(), costs.data(), supply.data(), sNds.data(), eNds.data());
    }
    catch (exception& ex)
    {
        LOGGER::LogFatal( ex.what() );
    }

    double totalCost = 0;
    spt->SetOrigin(orig);

    //No Dest setting -> routes to all dests will be computed
    try
    {
        LOGGER::LogDebug("Calculating routes from " + net.GetOriginalStartOrEndNodeID(orig) + " to all destinations in network..");
        // Solver fails on UNIT_MAX --> no dest setting at all!
        spt->SetDest( UINT_MAX );
        LOGGER::LogDebug("Starting to solve SPT..");
        spt->ShortestPathTree();
        LOGGER::LogDebug("SPT solved!");
    }
    catch (exception& ex)
    {
        //spt.Dispose();
        //throw new ApplicationException(ex.Message, ex.InnerException);
    }

    nmax = spt->MCFnmax();
    anz = spt->MCFnmax();

    //vector<unsigned int> a_pre;
    vector<unsigned int> pre;
    vector<unsigned int> nodes;

    //a_pre.reserve(anz + 1);
    pre.reserve(anz + 1);
    nodes.reserve(anz + 1);

    try
    {
        // vector::data() returns pointer
        //spt->GetArcPredecessors(a_pre.data());
        spt->GetPredecessors(pre.data());
    }
    catch (exception& ex)
    {
        //spt.Dispose();
        //throw new ApplicationException(ex.Message, ex.InnerException);
    }

    unordered_map<unsigned int,unsigned int> arcs;
    arcs.reserve(spt->MCFmmax());
    for (int i = nmax; i > 0; i--)
    {
        //if (a_pre[i] != ign[0] && a_pre[i] != ign[1])
        if (pre[i] != ign[0] && pre[i] != ign[1])
            arcs.insert( make_pair(i,pre[i]));
    }

    // Direction is irrelevant - the solver deals with the direction.
    // Thus it's not necessary to pay attention at this when building the route
    // out of the predecessors.
    reachedDests.clear();
    shortestPaths.clear();
    vector<unsigned int> route;

    // Get all routes from orig to dest in nodes-List
    for (unsigned int dest : dests)
    {
        bool isDestReached = spt->Reached(dest);

        if (orig != dest && isDestReached)
        {
            //route is reference
            const double costPerRoute = buildCompressedRoute(route, orig, dest, arcs);
            totalCost += costPerRoute;

            //Neuer vector muss sein, wegen clear() Methode weiter unten - sonst werden
            // bei sps auch die Vektoren geleert.
            shortestPaths.insert( make_pair( ODPair {orig, dest},
                                  make_pair( route, costPerRoute ) ) );
                                            //vector<unsigned int> (route),
                                             //   costPerRoute)) );
            route.clear();
            reachedDests.push_back(dest);
        }
        if (!isDestReached)
        {
            LOGGER::LogError("Destination "+ net.GetOriginalStartOrEndNodeID(dest) +" unreachable!");
        }
    }
    optimum = totalCost;
    //spt.Dispose();
}

SPTAlgorithm ShortestPathTree::GetAlgorithm() const
{
    return this->algorithm;
}
void ShortestPathTree::SetAlgorithm(SPTAlgorithm sptAlgorithm)
{
    this->algorithm = sptAlgorithm;
}

GEOMETRY_HANDLING ShortestPathTree::GetGeometryHandling() const
{
    return this->geometryHandling;
}
void ShortestPathTree::SetGeometryHandling(GEOMETRY_HANDLING geomHandling)
{
    this->geometryHandling = geomHandling;
}

int ShortestPathTree::GetSPTHeapCard() const
{
    return this->sptHeapCard;
}
void ShortestPathTree::SetSPTHeapCard(int heapCard)
{
    this->sptHeapCard = heapCard;
}

unsigned int ShortestPathTree::GetOrigin() const
{
    return this->originNode;
}
void ShortestPathTree::SetOrigin(unsigned int orig)
{
    this->originNode = orig;
}

vector<unsigned int> ShortestPathTree::GetDestinations() const
{
    return this->destinationNodes;
}
void ShortestPathTree::SetDestinations(vector<unsigned int>& dests)
{
    this->destinationNodes = dests;
}
vector<unsigned int> ShortestPathTree::GetReachedDests() const
{
    return this->reachedDests;
}
unordered_map<ODPair, CompressedPath> ShortestPathTree::GetShortestPaths() const
{
    return this->shortestPaths;
}

double ShortestPathTree::GetOptimum() const {
    return this->optimum;
}

void ShortestPathTree::SaveResults(const std::string& resultTableName,
                                            const ColumnMap& cmap) const
{
    try
    {
        Config cnfg = this->NETXPERT_CNFG;
        unique_ptr<DBWriter> writer;
		unique_ptr<SQLite::Statement> qry; //is null in case of ESRI FileGDB
        switch (cnfg.ResultDBType)
        {
            case RESULT_DB_TYPE::SpatiaLiteDB:
            {
                if (cnfg.ResultDBPath == cnfg.NetXDBPath)
                {
                    //Override result DB Path with original netXpert DB path
                    writer = unique_ptr<DBWriter>(new SpatiaLiteWriter(cnfg, cnfg.NetXDBPath));
                }
                else
				{
                    writer = unique_ptr<DBWriter>(new SpatiaLiteWriter(cnfg));
				}
                writer->CreateNetXpertDB(); //create before preparing query
                writer->OpenNewTransaction();
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::ShortestPathTreeSolver, true);
                writer->CommitCurrentTransaction();
                /*if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
                {*/
                auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
                qry = unique_ptr<SQLite::Statement> (sldbWriter.PrepareSaveResultArc(resultTableName, NetXpertSolver::ShortestPathTreeSolver));
                //}
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
                writer->CreateNetXpertDB();
                writer->OpenNewTransaction();
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::ShortestPathTreeSolver, true);
                writer->CommitCurrentTransaction();
            }
                break;
        }

        LOGGER::LogDebug("Writing Geometries..");
        writer->OpenNewTransaction();

        std::string arcIDs = "";
        std::unordered_set<string> totalArcIDs;
        std::unordered_map<ODPair, CompressedPath>::const_iterator it; //const_iterator wegen Zugriff auf this->shortestPath?

		if (cnfg.GeometryHandling == GEOMETRY_HANDLING::RealGeometry)
		{
			LOGGER::LogDebug("Preloading relevant geometries into Memory..");

			#pragma omp parallel default(shared) private(it) num_threads(LOCAL_NUM_THREADS)
			{
				//populate arcIDs
				for (it = this->shortestPaths.begin(); it != this->shortestPaths.end(); ++it)
				{
					#pragma omp single nowait
					{
						auto kv = *it;
						ODPair key = kv.first;
						CompressedPath value = kv.second;
						std::vector<unsigned int> ends = value.first;
						std::vector<InternalArc> route;
						std::unordered_set<std::string> arcIDlist;

						route = this->UncompressRoute(key.origin, ends);
						arcIDlist = this->net->GetOriginalArcIDs(route, cnfg.IsDirected);

						if (arcIDlist.size() > 0)
						{
							#pragma omp critical
							{
								for (std::string id : arcIDlist)
								totalArcIDs.insert(id);
							}
						}
					}//omp single
				}
			}//omp parallel

			for (string id : totalArcIDs)
				arcIDs += id += ",";

			if (arcIDs.size() > 0)
			{
				arcIDs.pop_back(); //trim last comma
				DBHELPER::LoadGeometryToMem(cnfg.ArcsTableName, cmap, cnfg.ArcsGeomColumnName, arcIDs);
			}
			LOGGER::LogDebug("Done!");
		}
        int counter = 0;

		#pragma omp parallel shared(counter) private(it) num_threads(LOCAL_NUM_THREADS)
        {

        for (it = this->shortestPaths.begin(); it != this->shortestPaths.end(); ++it)
        {
            #pragma omp single nowait
            {
            auto kv = *it;

            counter += 1;
            if (counter % 2500 == 0)
                LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

            string arcIDs = "";
            ODPair key = kv.first;
            CompressedPath value = kv.second;
            vector<unsigned int> ends = value.first;
            double costPerPath = value.second;
            vector<InternalArc> route;
            unordered_set<string> arcIDlist;

            route = this->UncompressRoute(key.origin, ends);
            arcIDlist = this->net->GetOriginalArcIDs(route, cnfg.IsDirected);

            if (arcIDlist.size() > 0)
            {
                for (string id : arcIDlist)
                    arcIDs += id += ",";
                arcIDs.pop_back(); //trim last comma
            }

            string orig = "";
            string dest = "";
            try{
                orig = this->net->GetOriginalStartOrEndNodeID(key.origin);
            }
            catch (exception& ex) {
                try {
                    orig = this->net->GetOriginalNodeID(key.origin);
                }
                catch (exception& ex){
                    orig = to_string(key.origin);
                }
            }
            try{
                dest = this->net->GetOriginalStartOrEndNodeID(key.dest);
            }
            catch (exception& ex) {
                try {
                    dest = this->net->GetOriginalNodeID(key.dest);
                }
                catch (exception& ex){
                    dest = to_string(key.dest);
                }
            }
            this->net->ProcessSPTResultArcsMem(orig, dest, costPerPath, arcIDs, route, resultTableName, *writer, *qry);
            }//omp single
        }
        }//omp paralell

        writer->CommitCurrentTransaction();
        writer->CloseConnection();
        LOGGER::LogDebug("Done!");
    }
    catch (exception& ex)
    {
        LOGGER::LogError("ShortestPaths::SaveResults() - Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}

vector<InternalArc> ShortestPathTree::UncompressRoute(unsigned int orig, vector<unsigned int>& ends) const
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

bool internalArcComp(std::pair<netxpert::data::InternalArc,netxpert::data::ArcData> a,
                            std::pair<netxpert::data::InternalArc,netxpert::data::ArcData> b) {
    return a.first.fromNode > b.first.fromNode;
}

bool sortedArcComp(std::pair<netxpert::data::InternalArc,netxpert::data::ArcData> a,
                            std::pair<netxpert::data::InternalArc,netxpert::data::ArcData> b) {
    return  (a.first == b.first) ;
}


void ShortestPathTree::convertInternalNetworkToSolverData(Network& net, vector<unsigned int>& sNds,
            vector<unsigned int>& eNds, vector<double>& supply, vector<double>& caps, vector<double>& costs)
{
    // Die Größe der Arrays müssen passen (ob 0 drin steht ist egal, sonst gibts später bei Dispose
    // das böse Erwachen in Form eines Heap Corruption errors bzw. einer System Access Violation

    Arcs arcs = net.GetInternalArcData();

    //sort per fromNode for static graph of LEMON
    netxpert::utils::LOGGER::LogDebug("Start sorting arcs..");
    std::vector< std::pair<InternalArc, ArcData> > sortedArcsVec (arcs.begin(), arcs.end());
    std::sort(sortedArcsVec.begin(), sortedArcsVec.end(), internalArcComp);
    netxpert::utils::LOGGER::LogDebug("End of sorting.");
    arcs.clear();

    Arcs sortedArcs (sortedArcsVec.begin(), sortedArcsVec.end());
    sortedArcsVec.clear();

    vector<InternalArc> keys;
    for(Arcs::iterator it = sortedArcs.begin(); it != sortedArcs.end(); ++it) {
      keys.push_back(it->first);
    }

    /*for(std::vector< std::pair<InternalArc, ArcData> >::const_iterator it = sortedArcs.begin(); it != sortedArcs.end(); ++it) {
      keys.push_back(it->first);
    }*/

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
        if (sortedArcs.count(keys[i]) > 0)
            oldArcData = sortedArcs.at(keys[i]);
        costs[i] = oldArcData.cost;
        //SPTree does not care about capacity
        //caps[i] = 0; //oldArcData.capacity;
    }

    /*for (int i = 0; i < keys.size(); i++)
    {
        ArcData oldArcData;
        //binary search for ordered elements - find for unordered
        //but we use find_if for getting the element if it has been found
        auto key = keys[i];
        std::vector< std::pair<InternalArc, ArcData> >::const_iterator element =
                std::find_if(sortedArcs.begin(), sortedArcs.end(),
                //lamdba for comparing the keys only
                [key](const std::pair<InternalArc,ArcData>& element)
                {
                    return element.first == key;
                });


        if ( element != sortedArcs.end() )
            oldArcData = element->second;
        costs[i] = oldArcData.cost;
        //SPTree does not care about capacity
        //caps[i] = 0; //oldArcData.capacity;
    }*/

    supply.resize( net.GetMaxNodeCount(), 0 ); //Größe muss passen!
}

bool ShortestPathTree::validateNetworkData(Network& net, unsigned int orig)
{
    bool valid = false;
    //
    cout << "Orig: "<<orig << endl;
    //IMPORTANT Checks
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

    valid = true;
    return valid;
}

bool ShortestPathTree::validateNetworkData(Network& net, unsigned int orig, unsigned int dest)
{
    bool valid = false;

    //IMPORTANT Checks
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
    if (dest > net.GetMaxNodeCount() ) {
            //throw new InvalidValueException("Destination Node ID must not exceed net.MaxNodeCount!");
            LOGGER::LogFatal("Destination Node ID must not exceed maximum node count of network!");
            throw;
    }

    valid = true;
    return valid;
}
bool ShortestPathTree::validateNetworkData(Network& net, unsigned int orig, vector<unsigned int>& dests)
{
    bool valid = false;

    //IMPORTANT Checks
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

double ShortestPathTree::buildCompressedRoute(vector<unsigned int>& route, unsigned int orig, unsigned int dest,
                                                unordered_map<unsigned int, unsigned int>& arcPredescessors)
{
    double totalCost = 0;
    //Neu - nur die Enden hinzufuegen
    unsigned int curr = dest;
    //cout << "orig " << orig << endl;
    while (curr != orig)
    {
        route.push_back(curr);
        const InternalArc& ftNode { arcPredescessors.at(curr), curr };
        totalCost += getArcCost( ftNode );
        curr = arcPredescessors.at(curr);
    }
    //arcPredescessors = null;
    return totalCost;
}
double ShortestPathTree::getArcCost(const InternalArc& arc)
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
