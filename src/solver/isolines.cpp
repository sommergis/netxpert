#include "isolines.h"

using namespace std;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::data;
using namespace netxpert::io;
using namespace netxpert::core;
using namespace netxpert::utils;

Isolines::Isolines(Config& cnfg)
{
    //ctor
    LOGGER::LogInfo("Isolines Solver instantiated");
    algorithm = cnfg.SptAlgorithm;
    isDirected = cnfg.IsDirected;
    sptHeapCard = cnfg.SPTHeapCard;
    geometryHandling = cnfg.GeometryHandling;
    this->NETXPERT_CNFG = cnfg;
}

void Isolines::Solve(std::string net)
{
    throw;
}

void Isolines::Solve(Network& net)
{
    this->net = &net;

    uint32_t arcCount = net.GetCurrentArcCount();
    uint32_t nodeCount = net.GetCurrentNodeCount();

    if (this->originNodes.size() == 0)
    {
        LOGGER::LogFatal("Origin Nodes in Isolines Solver must not be null!");
        throw;
    }
    else if (this->originNodes.size() > 0)
    {
        //set the size for the heap to m/n (arcs/nodes) if -1
        checkSPTHeapCard(arcCount, nodeCount);
        //pass the vector with the internal originNodes only
        std::vector<uint32_t> nodes;
        for (auto kv : this->originNodes)
        {
            nodes.push_back(kv.first);
        }
        solve(net, nodes, isDirected);
    }
}


void Isolines::SaveResults(const std::string& resultTableName,
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
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::IsolinesSolver, true);
                writer->CommitCurrentTransaction();
                /*if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
                {*/
                auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
                qry = unique_ptr<SQLite::Statement> (sldbWriter.PrepareSaveResultArc(resultTableName, NetXpertSolver::IsolinesSolver));
                //}
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
                writer->CreateNetXpertDB();
                writer->OpenNewTransaction();
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::IsolinesSolver, true);
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
            //TODO: costPerPath relative to geom length
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

            this->net->ProcessIsoResultArcsMem(orig, costPerPath, arcIDs, route,
                                            resultTableName, *writer, *qry, this->cutOffs);
            }//omp single
        }
        }//omp paralell
        LOGGER::LogDebug("Committing..");
        writer->CommitCurrentTransaction();
        writer->CloseConnection();
        LOGGER::LogDebug("Done!");
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Isolines::SaveResults() - Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}




void Isolines::checkSPTHeapCard(uint32_t arcCount, uint32_t nodeCount)
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
* n - all
*/
void Isolines::solve (Network& net, std::vector<uint32_t> origs, bool isDirected)
{
    //vector<long> ign = { 0, -1, (long)4294967295, (long)4261281277 };
    // 0 is a valid ignore value for linux!
    //TODO check in windows
    vector<long> ign  { -1, (long)4294967295, (long)4261281277 };

    uint32_t nmax;
    uint32_t anz;
    /*vector<uint32_t> a_pre;
    vector<uint32_t> pre;
    vector<uint32_t> nodes;*/

    vector<uint32_t> sNds;
    vector<uint32_t> eNds;
    vector<double> supply;
    vector<double> costs;
    vector<double> caps;

    if (!validateNetworkData( net, origs ))
        //throw new InvalidValueException(string.Format("Problem data does not fit the {0} Solver!", this.ToString()));
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetMaxArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetMaxNodeCount() ));
    LOGGER::LogDebug("Origs: "+ to_string(origs.size()));
    LOGGER::LogDebug("Solving..");

    //Read the network
    convertInternalNetworkToSolverData(net, sNds, eNds, supply, caps, costs);

    double totalCost = 0;

    //Main loop for calculating the SPTs
    int counter = 0;
    auto origsSize = origs.size();

    //#pragma omp parallel for shared(origs)
    //make spt local to be copied for parallel proc
    //no class member variables can be parallized
	#pragma omp parallel default(shared) num_threads(LOCAL_NUM_THREADS)
    {
    shared_ptr<ISPTree> lspt;
    switch (algorithm)
    {
        case SPTAlgorithm::Dijkstra_2Heap_LEMON:
            lspt = shared_ptr<ISPTree>(new SPT_LEM(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
            break;
        case SPTAlgorithm::Bijkstra_2Heap_LEMON:
            lspt = shared_ptr<ISPTree>(new SPT_LEM_Bijkstra_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                    isDirected));
            break;
        default:
            lspt = shared_ptr<ISPTree>(new SPT_LEM(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
            break;
    }

    lspt->LoadNet( static_cast<uint32_t>( net.GetMaxNodeCount() ), static_cast<uint32_t>( net.GetMaxArcCount()),
                        static_cast<uint32_t>( net.GetMaxNodeCount() ),static_cast<uint32_t>( net.GetMaxArcCount() ),
                        caps.data(), costs.data(), supply.data(), sNds.data(), eNds.data());

    nmax = lspt->MCFnmax();
    anz = lspt->MCFnmax();

    //vector<uint32_t> a_pre;
    vector<uint32_t> pre;
    vector<uint32_t> nodes;

    //a_pre.reserve(anz + 1);
    pre.reserve(anz + 1);
    nodes.reserve(anz + 1);

    vector<uint32_t>::iterator origIt;

    for (origIt = origs.begin(); origIt!=origs.end(); origIt++)
    {
        #pragma omp single nowait
        {
        uint32_t orig = *origIt;

        lspt->SetOrigin(orig);

        int routesLeft = origsSize - counter;
        LOGGER::LogDebug("# "+ to_string(omp_get_thread_num())  +": Calculating routes from " + net.GetOriginalStartOrEndNodeID(orig) + ", # "+
                            to_string(routesLeft) +" left..");
        // Set dests to UINT_MAX --> no dest setting at all!
        lspt->SetDest(UINT_MAX);
        //LOGGER::LogDebug("Starting to solve SPT..");

        lspt->ShortestPathTree();
        //LOGGER::LogDebug("SPT solved! ");

        //lspt->GetArcPredecessors(a_pre.data());
        lspt->GetPredecessors(pre.data());

        //LOGGER::LogDebug("before arcs preprocessing");

        unordered_map<uint32_t,uint32_t> arcs;
        int i;
        arcs.reserve(lspt->MCFmmax());
        //omp: local arcs --> no concurrent writes per thread
        for (i = nmax; i > 0; i--)
        {
            if (pre[i] != ign[0] && pre[i] != ign[1])
                arcs.insert( make_pair(i,pre[i]) );
        }
        //LOGGER::LogDebug("arcs preprocessed");

        //Compute nodes vector
        for (int i = 1; i < anz + 1; i++) {
            nodes.push_back( (uint32_t)i );
            //std::cout << i << std::endl;
        }


        vector<uint32_t> route;
        vector<uint32_t>::iterator nodesIt;
        for (nodesIt = nodes.begin(); nodesIt != nodes.end(); nodesIt++)
        {
            auto dest = *nodesIt;

            if (orig != dest && lspt->Reached(dest))
            {
                //LOGGER::LogDebug("orig: "+to_string(orig) + " dest: "+to_string(dest) );
                //route is reference
                const double costPerRoute = buildCompressedRoute(route, orig, dest, arcs);

                //LOGGER::LogDebug("route compressed");
                totalCost += costPerRoute;

                #pragma omp critical
                {
                this->shortestPaths.insert( make_pair( ODPair {orig, dest},
                                      make_pair( route, costPerRoute ) ));
                //LOGGER::LogDebug("sp saved internally");
                route.clear();
                }
            }
        }
        optimum = totalCost; //totalCost wird immer weiter aufsummiert für jedes neues OD-Paar
        counter += 1;

    }//omp single
    }
    }//omp parallel
}

double Isolines::GetOptimum() const {
    return this->optimum;
}


vector<InternalArc> Isolines::UncompressRoute(uint32_t orig, vector<uint32_t>& ends) const
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

void Isolines::convertInternalNetworkToSolverData(Network& net, vector<uint32_t>& sNds,
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
}


bool Isolines::validateNetworkData(Network& net, vector<uint32_t>& origs)
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

    valid = true;
    return valid;
}

double Isolines::buildCompressedRoute(vector<uint32_t>& route, uint32_t orig, uint32_t dest,
                                                unordered_map<uint32_t, uint32_t>& arcPredescessors)
{
    double totalCost = 0;
    //Neu - nur die Enden hinzufuegen
    uint32_t curr = dest;
    /* //cout << "orig " << orig << endl;
    //TODO: implement break if cutoff is reached
    //funktioniert nicht, weil der Pfad von dest zu orig aufgelöst wird

    cout << "Getting orig node id "<< orig << endl;
    ExtNodeID extNodeID = this->net->GetOriginalStartOrEndNodeID(orig);
    double currentCutOff = this->cutOffs.at(extNodeID);

    cout << "Max dist: "<<currentCutOff << endl;*/
    while (curr != orig )// && (totalCost < currentCutOff) )
    {
        route.push_back(curr);
        const InternalArc& ftNode { arcPredescessors.at(curr), curr };
        totalCost += getArcCost( ftNode );
        curr = arcPredescessors.at(curr);
    }
    return totalCost;
}
double Isolines::getArcCost(const InternalArc& arc)
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
std::map<ExtNodeID, std::vector<double> > Isolines::GetCutOffs()
{
    std::map<ExtNodeID, std::vector<double> > result;
    for (auto kv : this->cutOffs)
    {
        result.insert(make_pair(kv.first, kv.second));
    }
    return result;
}

void Isolines::SetCutOffs(std::map<ExtNodeID, std::vector<double> >& cutOffs)
{
    std::unordered_map<ExtNodeID, std::vector<double> > tempMap;
    for (auto kv : cutOffs)
    {
        tempMap.insert(make_pair(kv.first, kv.second));
    }
    this->cutOffs = tempMap;
}

std::unordered_map<ODPair, CompressedPath> Isolines::GetShortestPaths() const
{
    return this->shortestPaths;
}

SPTAlgorithm Isolines::GetAlgorithm() const
{
    return this->algorithm;
}
void Isolines::SetAlgorithm(SPTAlgorithm sptAlgorithm)
{
    this->algorithm = sptAlgorithm;
}

GEOMETRY_HANDLING Isolines::GetGeometryHandling() const
{
    return this->geometryHandling;
}
void Isolines::SetGeometryHandling(GEOMETRY_HANDLING geomHandling)
{
    this->geometryHandling = geomHandling;
}

int Isolines::GetSPTHeapCard() const
{
    return this->sptHeapCard;
}
void Isolines::SetSPTHeapCard(int heapCard)
{
    this->sptHeapCard = heapCard;
}

std::vector< std::pair<uint32_t,std::string> > Isolines::GetOrigins() const
{
    return this->originNodes;
}

void Isolines::SetOrigins(std::vector< std::pair<uint32_t,std::string> >& origs)
{
    this->originNodes = origs;
}
