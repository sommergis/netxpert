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

void
 Isolines::Solve(std::string net) {
    throw;
}

void
 Isolines::Solve(netxpert::InternalNet& net) {

    LOGGER::LogInfo("Using # " + to_string(LOCAL_NUM_THREADS) + " threads.");

    this->net = &net;

    uint32_t arcCount = net.GetArcCount();
    uint32_t nodeCount = net.GetNodeCount();

    if (this->originNodes.size() == 0)
    {
        LOGGER::LogFatal("Origin Nodes in Isolines Solver must not be null!");
        throw;
    }
    else if (this->originNodes.size() > 0)
    {
        //set the size for the heap to m/n (arcs/nodes) if -1
        checkSPTHeapCard(arcCount, nodeCount);
        solve(net, this->originNodes, isDirected);
    }
}

void
 Isolines::checkSPTHeapCard(uint32_t arcCount, uint32_t nodeCount) {

    if (sptHeapCard == -1)
    {
        if (isDirected) {
            sptHeapCard = (int) (arcCount / nodeCount) ;
        }
        else {
            sptHeapCard = (int) (arcCount * 2 / nodeCount );
        }
    }
}

/**
* n - all
*/
void Isolines::solve (netxpert::InternalNet& net, std::vector<netxpert::data::node_t> origs, bool isDirected)
{

    if (!validateNetworkData( net, origs ))
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetNodeCount() ));
    LOGGER::LogDebug("Origs: "+ to_string(origs.size()));
    LOGGER::LogDebug("Solving..");

    netxpert::data::cost_t totalCost = 0;


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
        /*case SPTAlgorithm::Dijkstra_2Heap_LEMON:
            lspt = shared_ptr<ISPTree>(new SPT_LEM(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                        isDirected));
            break;
        case SPTAlgorithm::Bijkstra_2Heap_LEMON:
            lspt = shared_ptr<ISPTree>(new SPT_LEM_Bijkstra_2Heap(net.GetMaxNodeCount(), net.GetMaxArcCount(),
                                    isDirected));
            break;*/
        default:
            lspt = shared_ptr<ISPTree>(new SPT_LEM(isDirected));
            break;
    }

    //Read the network
    auto sg = convertInternalNetworkToSolverData(net);
    lspt->LoadNet(net.GetNodeCount(), net.GetArcCount(), &sg, net.GetCostMap());

    vector<netxpert::data::node_t>::iterator origIt;

    for (origIt = origs.begin(); origIt!=origs.end(); origIt++)
    {
        #pragma omp single nowait
        {
        netxpert::data::node_t orig = *origIt;

        lspt->SetOrigin(orig);

        int routesLeft = origsSize - counter;
        LOGGER::LogDebug("# "+ to_string(omp_get_thread_num())  +": Calculating routes from " + to_string(net.GetNodeID(orig)) + ", # "+
                            to_string(routesLeft) +" left..");
        // Set dests to lemon::INVALID --> no dest setting at all!
        lspt->SetDest(lemon::INVALID);

        //set treshold to max of cut off for this node
        auto cuts = this->cutOffs.at(this->net->GetOrigNodeID(orig));
        double maxCut = (*std::max_element(cuts.begin(), cuts.end()));
        lspt->SolveSPT(maxCut, false);
        //LOGGER::LogDebug("SPT solved! ");

        // Get all routes from orig to dest in nodes-List
        auto nodesIter = this->net->GetNodesIter();
        //auto* g = this->net->GetGraph();
        //netxpert::data::graph_t::NodeIt nodesIter(*g);
        for ( ; nodesIter != lemon::INVALID; ++nodesIter)
        {
            auto dest = nodesIter;

            if (orig != dest && lspt->Reached(dest))
            {
                const auto costPerRoute = lspt->GetDist(dest);
                const auto path = lspt->GetPath(dest);
                totalCost = totalCost + costPerRoute;

                #pragma omp critical
                {
                this->shortestPaths.insert( make_pair( ODPair {orig, dest},
                                   make_pair( path, costPerRoute)) );
                //LOGGER::LogDebug("sp saved internally");
                }
            }
        }
        optimum = totalCost; //totalCost wird immer weiter aufsummiert für jedes neues OD-Paar
        counter += 1;

    }//omp single
    }
    }//omp parallel
}

SPTAlgorithm
 Isolines::GetAlgorithm() const {
    return this->algorithm;
}

inline const double Isolines::GetOptimum() const {
    return this->optimum;
}

void
 Isolines::SetAlgorithm(SPTAlgorithm sptAlgorithm) {
    this->algorithm = sptAlgorithm;
}

GEOMETRY_HANDLING
 Isolines::GetGeometryHandling() const {
    return this->geometryHandling;
}

void
Isolines::SetGeometryHandling(GEOMETRY_HANDLING geomHandling) {
    this->geometryHandling = geomHandling;
}

int
 Isolines::GetSPTHeapCard() const {
    return this->sptHeapCard;
}

void
 Isolines::SetSPTHeapCard(int heapCard) {
    this->sptHeapCard = heapCard;
}

std::vector<netxpert::data::node_t>
 Isolines::GetOrigins() const {
    return this->originNodes;
}

void
 Isolines::SetOrigins(std::vector<netxpert::data::node_t>& origs) {
    this->originNodes = origs;
}

map<ODPair, CompressedPath>
 Isolines::GetShortestPaths() const {
    return this->shortestPaths;
}

void
 Isolines::SaveResults(const std::string& resultTableName,
                            const ColumnMap& cmap) const {
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

        //Processing and Saving Results are handled within net.ProcessResultArcs()
        std::string arcIDs = "";
        std::unordered_set<string> totalArcIDs;
        std::map<ODPair, CompressedPath>::const_iterator it; //const_iterator wegen Zugriff auf this->shortestPath

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
//						ODPair key = kv.first;
						CompressedPath value = kv.second;
						/* resolve pred path to arcids */
						/* ArcLookup vs AllArcLookup vs saving the path of the route, not only the preds ?*/
						auto path = value.first;

						std::unordered_set<std::string> arcIDlist = this->net->GetOrigArcIDs(path);

						if (arcIDlist.size() > 0)
						{
							#pragma omp critical
							{
								for (std::string id : arcIDlist){
								  totalArcIDs.insert(id);
								}
							}
						}
					}//omp single
				}
			}//omp parallel

			for (string id : totalArcIDs) {
				arcIDs += id += ",";
            }

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
            /* resolve pred path to arcids */
            /* ArcLookup vs AllArcLookup vs saving the path of the route, not only the preds ?*/
            auto path = value.first;
            std::unordered_set<std::string> arcIDlist = this->net->GetOrigArcIDs(path);
            double costPerPath = value.second;

            if (arcIDlist.size() > 0)
            {
                for (string id : arcIDlist) {
                    arcIDs += id += ",";
                }
                arcIDs.pop_back(); //trim last comma
            }

            string orig = this->net->GetOrigNodeID(key.origin);
//            std::cout << "orig " << orig << std::endl;

            this->net->ProcessIsoResultArcsMem(orig, costPerPath, arcIDs, path,
                                               resultTableName, *writer, *qry, this->cutOffs);
            }//omp single
        }

        LOGGER::LogDebug("AVG Geo Proc Time: "+ to_string(avgGeoProcTime/counter));
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

lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
 Isolines::convertInternalNetworkToSolverData(InternalNet& net
                                            /*lemon::FilterArcs<netxpert::data::graph_t,
                                                              lemon::IterableBoolMap<netxpert::data::graph_t,
                                                                                     netxpert::data::arc_t>
                                                              >& arcFilter //OUT*/
                                                            )
{
    using namespace netxpert::data;
    LOGGER::LogInfo("#Arcs internal graph: " +to_string(lemon::countArcs(*net.GetGraph())));

    lemon::FilterArcs<graph_t, graph_t::ArcMap<bool>> sg(*net.GetGraph(), *net.GetArcFilterMap());

    assert(lemon::countArcs(sg) > 0);

    LOGGER::LogInfo("#Arcs filtered graph: " +to_string(lemon::countArcs(sg)));
    return sg;
}

bool
 Isolines::validateNetworkData(netxpert::InternalNet& net, vector<netxpert::data::node_t>& origs) {
    bool valid = false;

    //IMPORTANT Checks
    /*for (auto orig : origs)
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
    }*/

    valid = true;
    return valid;
}

std::map<ExtNodeID, std::vector<double> >
 Isolines::GetCutOffs() {
    std::map<ExtNodeID, std::vector<double> > result;
    for (auto kv : this->cutOffs)
    {
        result.insert(make_pair(kv.first, kv.second));
    }
    return result;
}

void
 Isolines::SetCutOffs(std::map<ExtNodeID, std::vector<double> >& cutOffs) {
    std::unordered_map<ExtNodeID, std::vector<double> > tempMap;
    for (auto kv : cutOffs)
    {
        tempMap.insert(make_pair(kv.first, kv.second));
    }
    this->cutOffs = tempMap;
}



