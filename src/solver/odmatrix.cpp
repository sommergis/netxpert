/*
 * This file is a part of netxpert.
 *
 * Copyright (C) 2013-2017
 * Johannes Sommer, Christopher Koller
 *
 * Permission to use, modify and distribute this software is granted
 * provided that this copyright notice appears in all copies. For
 * precise terms see the accompanying LICENSE file.
 *
 * This software is provided "AS IS" with no warranty of any kind,
 * express or implied, and with no claim as to its suitability for any
 * purpose.
 *
 */

#include "odmatrix.h"

using namespace std;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::data;
using namespace netxpert::io;
using namespace netxpert::core;
using namespace netxpert::utils;

OriginDestinationMatrix::OriginDestinationMatrix(Config& cnfg)
{
    //ctor
    LOGGER::LogInfo("OriginDestinationMatrix Solver instantiated");
    algorithm = cnfg.SptAlgorithm;
    isDirected = cnfg.IsDirected;
    sptHeapCard = cnfg.SPTHeapCard;
    geometryHandling = cnfg.GeometryHandling;
    this->NETXPERT_CNFG = cnfg;
}

void
 OriginDestinationMatrix::Solve(std::string net) {
    throw;
}

void
 OriginDestinationMatrix::Solve(netxpert::InternalNet& net) {

    LOGGER::LogInfo("Using # " + to_string(LOCAL_NUM_THREADS) + " threads.");

    this->net = &net;

    uint32_t arcCount = net.GetArcCount();
    uint32_t nodeCount = net.GetNodeCount();

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

void
 OriginDestinationMatrix::checkSPTHeapCard(uint32_t arcCount, uint32_t nodeCount) {

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
* n - n
*/
void
 OriginDestinationMatrix::solve (netxpert::InternalNet& net, vector<netxpert::data::node_t>& origs,
                                vector<netxpert::data::node_t>& dests, bool isDirected) {

    if (!validateNetworkData( net, origs, dests ))
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetNodeCount() ));
    LOGGER::LogDebug("Origs: "+ to_string(origs.size()));
    LOGGER::LogDebug("Dests: "+ to_string(net.GetNodeCount() ));
    LOGGER::LogDebug("Solving..");

    netxpert::data::cost_t totalCost = 0;

    //Main loop for calculating the ODMatrix
    int counter = 0;
    auto origsSize = origs.size();

    //for (auto orig : origs)
    /* OpenMP only wants iterator style in for-loops - no auto for loops
       The loop for the ODMatrix Calculation is being computed paralell with the critical keyword.
    */

    //shared(shortestPaths, odMatrix, reachedDests, origs)

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
        set< netxpert::data::node_t > intReachedDests;

        lspt->SetOrigin(orig);

        int routesLeft = origsSize - counter;
        LOGGER::LogDebug("# "+ to_string(omp_get_thread_num())  +": Calculating routes from " + to_string(net.GetNodeID(orig)) + ", # "+
                            to_string(routesLeft) +" left..");
        // Set dests to lemon::INVALID --> no dest setting at all!
        lspt->SetDest(lemon::INVALID);

        lspt->SolveSPT();
        //LOGGER::LogDebug("SPT solved! ");

        //omp: local intReachedDests --> no concurrent writes per thread
        for (auto dest : dests) {
            if (lspt->Reached(dest))
                intReachedDests.insert(dest);
        }

        // Get all routes from orig to dest in nodes-List
        vector<netxpert::data::node_t>::iterator destIt;
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

                const double costPerRoute = lspt->GetDist(dest);
                const auto path = lspt->GetPath(dest);

                #pragma omp critical
                {
                totalCost += costPerRoute;

                shortestPaths.insert( make_pair( ODPair {orig, dest},
                                      make_pair( path, costPerRoute ) ));//vector<uint32_t> (route),
                                                 //   costPerRoute)) );
                odMatrix.insert( make_pair( ODPair {orig, dest},
                                    costPerRoute));

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

SPTAlgorithm
 OriginDestinationMatrix::GetAlgorithm() const {
    return this->algorithm;
}

void
 OriginDestinationMatrix::SetAlgorithm(SPTAlgorithm sptAlgorithm) {
    this->algorithm = sptAlgorithm;
}

GEOMETRY_HANDLING
 OriginDestinationMatrix::GetGeometryHandling() const {
    return this->geometryHandling;
}
void
 OriginDestinationMatrix::SetGeometryHandling(GEOMETRY_HANDLING geomHandling)
{
    this->geometryHandling = geomHandling;
}

int
 OriginDestinationMatrix::GetSPTHeapCard() const {
    return this->sptHeapCard;
}

void
 OriginDestinationMatrix::SetSPTHeapCard(int heapCard) {
    this->sptHeapCard = heapCard;
}

vector<netxpert::data::node_t>
 OriginDestinationMatrix::GetOrigins() const {
    return this->originNodes;
}

void
 OriginDestinationMatrix::SetOrigins(vector<netxpert::data::node_t>& origs) {
    this->originNodes = origs;
}

void
 OriginDestinationMatrix::SetOrigins(vector<pair<netxpert::data::node_t, string>>& origs) {
    vector<netxpert::data::node_t> newOrigs;
    for (auto s : origs)
        newOrigs.push_back(s.first);
    this->originNodes = newOrigs;
}

vector<netxpert::data::node_t>
 OriginDestinationMatrix::GetDestinations() const {
    return this->destinationNodes;
}

vector<uint32_t>
 OriginDestinationMatrix::GetDestinationIDs() const {
    vector<uint32_t> result;
    for (auto n : this->destinationNodes) {
        result.push_back(this->net->GetNodeID(n));
    }
    return result;
}

void
 OriginDestinationMatrix::SetDestinations(vector<netxpert::data::node_t>& dests) {
    this->destinationNodes = dests;
}

void
 OriginDestinationMatrix::SetDestinations(vector<pair<netxpert::data::node_t, string>>& dests) {
    vector<netxpert::data::node_t> newDests;
    for (auto e : dests)
        newDests.push_back(e.first);
    this->destinationNodes = newDests;
}

vector<netxpert::data::node_t>
 OriginDestinationMatrix::GetReachedDests() const {
    return this->reachedDests;
}

vector<uint32_t>
 OriginDestinationMatrix::GetReachedDestIDs() const
{
    vector<uint32_t> result;
    for (auto n : this->reachedDests) {
        result.push_back(this->net->GetNodeID(n));
    }
    return result;
}

map<ODPair, CompressedPath>
 OriginDestinationMatrix::GetShortestPaths() const {
    return this->shortestPaths;
}

map<ODPair, double>
 OriginDestinationMatrix::GetODMatrix() const {
    return this->odMatrix;
}

inline const double
 OriginDestinationMatrix::GetOptimum() const {
    return this->optimum;
}

void
 OriginDestinationMatrix::SaveResults(const std::string& resultTableName,
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
            string dest = this->net->GetOrigNodeID(key.dest);

            this->net->ProcessSPTResultArcsMem(orig, dest, costPerPath, arcIDs, path, resultTableName, *writer, *qry);
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
        LOGGER::LogError("OriginDestinationMatrix::SaveResults() - Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}

lemon::FilterArcs<netxpert::data::graph_t, netxpert::data::graph_t::ArcMap<bool>>
 OriginDestinationMatrix::convertInternalNetworkToSolverData(InternalNet& net
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

bool OriginDestinationMatrix::validateNetworkData(netxpert::InternalNet& net,
                                                    vector<netxpert::data::node_t>& origs,
                                                    vector<netxpert::data::node_t>& dests)
{
    bool valid = false;

    //IMPORTANT Checks
    /*for (uint32_t orig : origs)
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
    }*/

    valid = true;
    return valid;
}
