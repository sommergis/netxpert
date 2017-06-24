#include "odmatrix2.h"

using namespace std;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::data;
using namespace netxpert::io;
using namespace netxpert::core;
using namespace netxpert::utils;

OriginDestinationMatrix2::OriginDestinationMatrix2(Config& cnfg)
{
    //ctor
    LOGGER::LogInfo("OriginDestinationMatrix2 Solver instantiated");
    algorithm = cnfg.SptAlgorithm;
    isDirected = cnfg.IsDirected;
    sptHeapCard = cnfg.SPTHeapCard;
    geometryHandling = cnfg.GeometryHandling;
    this->NETXPERT_CNFG = cnfg;
}

void OriginDestinationMatrix2::Solve(string net){
    throw;
}

void OriginDestinationMatrix2::Solve(Network& net)
{
    this->net = &net;

    uint32_t arcCount = net.GetCurrentArcCount();
    uint32_t nodeCount = net.GetCurrentNodeCount();

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
void OriginDestinationMatrix2::checkSPTHeapCard(uint32_t arcCount, uint32_t nodeCount)
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
void OriginDestinationMatrix2::solve (Network& net, vector<uint32_t>& origs,
                                vector<uint32_t>& dests, bool isDirected)
{
    //vector<long> ign = { 0, -1, (long)4294967295, (long)4261281277 };
    // 0 is a valid ignore value for linux!
    //TODO check in windows
    vector<long> ign  { -1, (long)4294967295, (long)4261281277 };

    uint32_t nmax;
    uint32_t anz;

    vector<uint32_t> sNds;
    vector<uint32_t> eNds;
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

    //for (uint32_t orig : origs)
    /* OpenMP only wants iterator style in for-loops - no auto for loops
       The loop for the ODMatrix Calculation is being computed paralell with the critical keyword.
    */

    //#pragma omp parallel for shared(origs)
    //make spt local to be copied for parallel proc
    //no class member variables can be parallized

	//#pragma omp parallel default(shared) num_threads(LOCAL_NUM_THREADS)
    //{
    shared_ptr<ISPTree> lspt;
    switch (algorithm)
    {
        case SPTAlgorithm::ODM_LEM_2Heap:
            lspt = shared_ptr<ISPTree>(new netxpert::core::ODM_LEM_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
            break;
        default:
            lspt = shared_ptr<ISPTree>(new netxpert::core::ODM_LEM_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
            break;
    }

    lspt->LoadNet( static_cast<uint32_t>( net.GetMaxNodeCount() ), static_cast<uint32_t>( net.GetMaxArcCount()),
                        static_cast<uint32_t>( net.GetMaxNodeCount() ),static_cast<uint32_t>( net.GetMaxArcCount() ),
                        caps.data(), costs.data(), supply.data(), sNds.data(), eNds.data());

    nmax = lspt->MCFnmax();
    anz = lspt->MCFnmax();

    vector<uint32_t> pre;
    vector<uint32_t> nodes;

    pre.reserve(anz + 1);
    nodes.reserve(anz + 1);
    for (auto orig : this->originNodes)
        lspt->SetOrigin(orig);

    LOGGER::LogDebug("# "+ to_string(omp_get_thread_num())  +": Calculating routes for #" + to_string(this->originNodes.size())+ " routes.");

    // Set dests to UINT_MAX --> no dest setting at all!
    for (auto d : this->destinationNodes)
        lspt->SetDest(d);

    LOGGER::LogDebug("Starting to solve SPT..");
    lspt->ShortestPathTree();
    LOGGER::LogDebug("SPT solved! ");

    //auto& sldb = dynamic_cast<SpatiaLiteWriter&>(writer);
    auto odmSolver = dynamic_pointer_cast<netxpert::core::ODM_LEM_2Heap>(lspt);

    lspt->GetPredecessors(pre.data());

    unordered_map<uint32_t,uint32_t> arcs;
    arcs.reserve(lspt->MCFmmax());
    int i;
    //omp: local arcs --> no concurrent writes per thread
    for (i = nmax; i > 0; i--)
    {
        if (pre[i] != ign[0] && pre[i] != ign[1] && pre[i] > 0)
            arcs.insert( make_pair(i,pre[i]) );
    }

    // Direction is irrelevant - the solver deals with the direction.
    // Thus it's not necessary to pay attention at this when building the route
    // out of the predecessors.

    // Get all routes from orig to dest in nodes-List

    //for (uint32_t dest : dests)
    //{

    LOGGER::LogDebug("Arc predecessors: ");
    for (auto arc : arcs)
        LOGGER::LogDebug("Arc: " + to_string(arc.first) + "->" +to_string(arc.second));

    vector<uint32_t> route;
    for (auto orig : this->originNodes)
    {
        LOGGER::LogDebug("Orig: " + to_string(orig) + "..");
        for (auto dest : this->destinationNodes)
        {
            bool isDestReached = lspt->Reached(dest);

            if (orig != dest && isDestReached)
            {
                LOGGER::LogDebug("Dest: " + to_string(dest) + " is reached.");
                //odmSolver->GetPath(orig, dest);
                //route is reference
                try
                {
                    const double costPerRoute = buildCompressedRoute(route, orig, dest, arcs);
                    totalCost += costPerRoute;

                    //Neuer vector muss sein, wegen clear() Methode weiter unten - sonst werden
                    // bei sps auch die Vektoren geleert.
                    shortestPaths.insert( make_pair( ODPair {orig, dest},
                                          make_pair( route, costPerRoute ) ));//vector<uint32_t> (route),
                                                     //   costPerRoute)) );
                    odMatrix.insert( make_pair( ODPair {orig, dest},
                                        costPerRoute));
                    route.clear();
                    reachedDests.push_back(dest);
                }
                catch (std::exception& ex)
                {
                    LOGGER::LogWarning("Error finding Path!");
                }
                //}

            }
            if (!isDestReached)
            {
                //LOGGER::LogError("Destination "+ net.GetOriginalStartOrEndNodeID(dest) +" unreachable!");
            }
        }
    optimum = totalCost; //totalCost wird immer weiter aufsummiert für jedes neues OD-Paar
    counter += 1;
    }
}

SPTAlgorithm OriginDestinationMatrix2::GetAlgorithm() const
{
    return this->algorithm;
}
void OriginDestinationMatrix2::SetAlgorithm(SPTAlgorithm sptAlgorithm)
{
    this->algorithm = sptAlgorithm;
}

GEOMETRY_HANDLING OriginDestinationMatrix2::GetGeometryHandling() const
{
    return this->geometryHandling;
}
void OriginDestinationMatrix2::SetGeometryHandling(GEOMETRY_HANDLING geomHandling)
{
    this->geometryHandling = geomHandling;
}

int OriginDestinationMatrix2::GetSPTHeapCard() const
{
    return this->sptHeapCard;
}
void OriginDestinationMatrix2::SetSPTHeapCard(int heapCard)
{
    this->sptHeapCard = heapCard;
}

vector<uint32_t> OriginDestinationMatrix2::GetOrigins() const
{
    return this->originNodes;
}
void OriginDestinationMatrix2::SetOrigins(vector<uint32_t>& origs)
{
    this->originNodes = origs;
}
void OriginDestinationMatrix2::SetOrigins(vector<pair<uint32_t, string>>& origs)
{
    vector<uint32_t> newOrigs;
    for (auto s : origs)
        newOrigs.push_back(s.first);
    this->originNodes = newOrigs;
}

vector<uint32_t> OriginDestinationMatrix2::GetDestinations() const
{
    return this->destinationNodes;
}
void OriginDestinationMatrix2::SetDestinations(vector<uint32_t>& dests)
{
    this->destinationNodes = dests;
}

void OriginDestinationMatrix2::SetDestinations(vector<pair<uint32_t, string>>& dests)
{
    vector<uint32_t> newDests;
    for (auto e : dests)
        newDests.push_back(e.first);
    this->destinationNodes = newDests;
}
vector<uint32_t> OriginDestinationMatrix2::GetReachedDests() const
{
    return this->reachedDests;
}
unordered_map<ODPair, CompressedPath> OriginDestinationMatrix2::GetShortestPaths() const
{
    return this->shortestPaths;
}
unordered_map<ODPair, double> OriginDestinationMatrix2::GetODMatrix() const
{
    return this->odMatrix;
}

double OriginDestinationMatrix2::GetOptimum() const {
    return this->optimum;
}

void OriginDestinationMatrix2::SaveResults(const std::string& resultTableName,
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
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::ODMatrixSolver, true);
                writer->CommitCurrentTransaction();
                /*if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
                {*/
                auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
                qry = unique_ptr<SQLite::Statement> (sldbWriter.PrepareSaveResultArc(resultTableName, NetXpertSolver::ODMatrixSolver));
                //}
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
                writer->CreateNetXpertDB();
                writer->OpenNewTransaction();
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::ODMatrixSolver, true);
                writer->CommitCurrentTransaction();
            }
                break;
        }

        LOGGER::LogDebug("Writing Geometries..");
        writer->OpenNewTransaction();

        std::string arcIDs = "";
        std::unordered_set<string> totalArcIDs;
        std::unordered_map<ODPair, CompressedPath>::const_iterator it;

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
						std::vector<uint32_t> ends = value.first;
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

        Stopwatch<> sw;
        double avgGeoProcTime = 0;

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
            vector<uint32_t> ends = value.first;
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
            //sw.start();
            this->net->ProcessSPTResultArcsMem(orig, dest, costPerPath, arcIDs, route, resultTableName, *writer, *qry);
            //sw.stop();
            //avgGeoProcTime += sw.elapsed();
            }//omp single
        }

        LOGGER::LogDebug("AVG Geo Proc Time: "+ to_string(avgGeoProcTime/counter));
        }//omp paralell

        writer->CommitCurrentTransaction();
        writer->CloseConnection();
        LOGGER::LogDebug("Done!");
    }
    catch (exception& ex)
    {
        LOGGER::LogError("OriginDestinationMatrix2::SaveResults() - Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}

vector<InternalArc> OriginDestinationMatrix2::UncompressRoute(uint32_t orig, vector<uint32_t>& ends) const
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

void OriginDestinationMatrix2::convertInternalNetworkToSolverData(Network& net, vector<uint32_t>& sNds,
            vector<uint32_t>& eNds, vector<double>& supply, vector<double>& caps, vector<double>& costs)
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
        uint32_t key = item.first;
        NodeSupply value = item.second;
        // key is 1-based thus -1 for index
        // only care for real supply and demand values
        // transshipment nodes (=0) are already present in the array (because of new array)
        supply[key - 1] = value.supply;
    }*/
    //cout << "ready converting data" << endl;
}

bool OriginDestinationMatrix2::validateNetworkData(Network& net, vector<uint32_t>& origs, vector<uint32_t>& dests)
{
    bool valid = false;

    //IMPORTANT Checks
    for (uint32_t orig : origs)
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

double OriginDestinationMatrix2::buildCompressedRoute(vector<uint32_t>& route, uint32_t orig, uint32_t dest,
                                                unordered_map<uint32_t, uint32_t>& arcPredescessors)
{
    double totalCost = 0;
    //Neu - nur die Enden hinzufuegen
    uint32_t curr = dest;

    while (curr != orig)
    {
        LOGGER::LogDebug("Processing current node: " + to_string(curr));
        route.push_back(curr);
        const InternalArc& ftNode  { arcPredescessors.at(curr), curr };
        LOGGER::LogDebug("Arc: " + to_string(ftNode.fromNode) + "->" + to_string(ftNode.toNode));
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
double OriginDestinationMatrix2::getArcCost(const InternalArc& arc)
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

