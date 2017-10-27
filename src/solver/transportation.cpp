#include "transportation.h"

using namespace std;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::data;
using namespace netxpert::io;
using namespace netxpert::core;
using namespace netxpert::utils;


Transportation::Transportation(Config& cnfg) : MinCostFlow(cnfg)
{
    //ctor
    LOGGER::LogInfo("Transportation Solver instantiated");
    NETXPERT_CNFG = cnfg;
    algorithm = cnfg.McfAlgorithm;
    solverStatus = MCFSolverStatus::MCFUnSolved;
    IsDirected = true; //TODO CHECK always true
    this->NETXPERT_CNFG = cnfg;
}

Transportation::~Transportation()
{
// why does this lead to a invalid pointer memory error?
// only on Solve(net) not on Solve()
//    if (this->net)
//        delete this->net;
}

void
 Transportation::Solve() {

    if (this->extODMatrix.size() == 0 || this->extNodeSupply.size() == 0)
        throw std::runtime_error("OD-Matrix and node supply must be filled in Transportation Solver!");

    //arcData from ODMatrix
    InputArcs arcs;
    for (ExtSPTreeArc& v : extODMatrix)
    {
        ExtArcID key 	= v.extArcID;
        ExternalArc arc = v.extArc;
        cost_t cost 	= v.cost;
        capacity_t cap 	= DOUBLE_INFINITY; // TRANSPORTATION Problem, no caps
        string fromNode = arc.extFromNode;
        string toNode 	= arc.extToNode;
        string oneway = "";
        arcs.push_back( InputArc {key, fromNode, toNode, cost, cap, oneway } );
    }
    Config cnfg = this->NETXPERT_CNFG;
    ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

    //construct nodeSupply from external nodeSupply
    InputNodes nodes;
    for (ExtNodeSupply& v : extNodeSupply)
    {
        ExtNodeID key = v.extNodeID;
        supply_t supply = v.supply;
        nodes.push_back( InputNode {key, supply });
    }

    //construct network from external ODMatrix
    //TODO: checkme (pointer resource dealloc)
    //problem line, because this->net seems not to be persistent anymore
    // when used outside this scope
    this->net = new InternalNet (arcs, cmap, cnfg, nodes);

    //Check Balancing of Transportation Problem Instance and
    //transform the instance if needed is already done in
    //parent class method MinCostFlow::Solve()

    //Call MCF Solver of parent class
    MinCostFlow::Solve(*this->net);

    //2. Search for the ODpairs in ODMatrix Solver result with the original IDs of the
    // Min Cost Flow Solver result
    vector<FlowCost> flowCost = MinCostFlow::GetMinCostFlow();
    for (auto& fc : flowCost)
    {
        string oldStartNodeID;
        string oldEndNodeID;

        auto oldStartNode   = this->net->GetSourceNode(fc.intArc);
        auto oldEndNode     = this->net->GetTargetNode(fc.intArc);

        try
        {
            oldStartNodeID = this->net->GetOrigNodeID(oldStartNode);
            oldEndNodeID   = this->net->GetOrigNodeID(oldEndNode);

            if (oldStartNodeID != "dummy" && oldEndNodeID != "dummy")
            {
                //build key for result map
                ODPair resultKey { oldStartNode, oldEndNode };

                // there is no path, because we do not have the values from the OD solver
                DistributionArc resultVal { CompressedPath { make_pair(vector<arc_t> {}, fc.cost) }, fc.flow };

                this->distribution.insert(make_pair(resultKey, resultVal));
            }
            else
            {
                LOGGER::LogWarning("Dummy: FromTo "+
                                    oldStartNodeID + " - "+ oldEndNodeID+
                                    " could not be looked up!");
            }
        }
        catch (exception& ex)
        {
            LOGGER::LogError("Something strange happened - maybe a key error. ");
            LOGGER::LogError(ex.what());
        }
    }
}

void
 Transportation::Solve(netxpert::InternalNet& net) {

    this->net = &net;

    if (this->originNodes.size() == 0 || this->destinationNodes.size() == 0)
        throw std::runtime_error("Origin nodes and destination nodes must be filled in Transportation Solver!");

    //Call the ODMatrixSolver
    OriginDestinationMatrix ODsolver(this->NETXPERT_CNFG);
    //Precondition: Starts and Ends has been filled and thus
    // they were already converted to ascending ints
    ODsolver.SetOrigins(this->originNodes);
    ODsolver.SetDestinations(this->destinationNodes);
    ODsolver.Solve(net);
    this->odMatrix = ODsolver.GetODMatrix();

    std::set<netxpert::data::node_t> odKeys;

    //arcData from ODMatrix
    InputArcs arcs;
    int counter = 0;
    for (auto& kv : this->odMatrix)
    {
        counter += 1;
        ODPair key      = kv.first;
        auto cost       = kv.second;
        auto cap        = DOUBLE_INFINITY; // TRANSPORTATION Problem, no caps
        string fromNode = net.GetOrigNodeID(key.origin);
        string toNode   = net.GetOrigNodeID(key.dest);

//        std::cout << fromNode << "->" << toNode << "@" << cost << std::endl;

        string oneway = "";
        // We do not have a original ArcID - so we set counter as ArcID
        arcs.push_back( InputArc {to_string(counter), fromNode, toNode,
                                    cost, cap, oneway } );
        // Populate a unique key set of OD-Nodes
        if(odKeys.count(key.origin) == 0)
            odKeys.insert(key.origin);
        if(odKeys.count(key.dest) == 0)
            odKeys.insert(key.dest);
    }
    Config cnfg = this->NETXPERT_CNFG;
    ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

    // Add nodes data only, if reached through ODMatrix Solver
    InputNodes nodes;
//    auto* supplyMap = net.GetSupplyMap();
    auto nodesIter = net.GetNodesIter();

    for (; nodesIter != lemon::INVALID; ++nodesIter) {
        auto node       = nodesIter;
        supply_t supply = net.GetNodeSupply(node);
        // precondition: only reached nodes from ODMatrix solver shall be
        // processed
        if (odKeys.count(node) > 0) {
            nodes.push_back( InputNode {net.GetOrigNodeID(node), supply });
//            extIntNodeMap.insert (std::make_pair(net.GetOrigNodeID(node), net.GetNodeID(node)) );
        }
    }

//    std::map<std::string, IntNodeID> extIntNodeMap;
    // construct network from ODMatrix; the original nodes persist,
    // but the internal nodes will be computed new
    InternalNet mcfNet(arcs, cmap, cnfg, nodes);

//    nodesIter = net.GetNodesIter();
//    std::cout << " Net nodes " << std::endl;
//    for (; nodesIter != lemon::INVALID; ++nodesIter) {
//        std::cout << net.GetOrigNodeID(nodesIter) << " | " << net.GetNodeID(nodesIter) << std::endl;
//    }
//
//    std::cout << " mcfNet nodes " << std::endl;
//    auto nodesIter2 = mcfNet.GetNodesIter();
//    for (; nodesIter2 != lemon::INVALID; ++nodesIter2) {
//        std::cout << mcfNet.GetOrigNodeID(nodesIter2) << " | " << mcfNet.GetNodeID(nodesIter2) << std::endl;
//        std::cout << net.GetOrigNodeID( net.GetNodeFromOrigID(mcfNet.GetOrigNodeID(nodesIter2))  ) << " | "
//                  << net.GetOrigNodeID( net.GetNodeFromOrigID(mcfNet.GetOrigNodeID(nodesIter2)) ) << std::endl;
//    }
//
//    throw;

    //Check Balancing of Transportation Problem Instance and
    //transform the instance if needed is already done in
    //parent class method MinCostFlow::Solve()

    //Call MCF Solver of parent class
    MinCostFlow::Solve(mcfNet);

    //Go back from Origin->Dest: flow,cost to real path with a list of start and end nodes of the ODMatrix
    //1. Translate back to original node ids


    //2. Search for the ODpairs in ODMatrix Solver result with the original IDs of the
    // Min Cost Flow Solver result
    vector<FlowCost> flowCost                   = MinCostFlow::GetMinCostFlow(); //was run with mcfNet
    map<ODPair, CompressedPath> shortestPaths   = ODsolver.GetShortestPaths(); //was run with ODnet

//    std::cout << "ODMatrix result (int node ids): " <<std::endl;
//
//    for (auto& s : shortestPaths) {
//        std::cout << net.GetNodeID(s.first.origin) << " -> " << net.GetNodeID(s.first.dest)
//                  << " c: " << s.second.second << std::endl;
//    }
//    std::cout << "ODMatrix result (orig node ids): " <<std::endl;
//    for (auto& s : shortestPaths) {
//        std::cout << net.GetOrigNodeID(s.first.origin) << " -> " << net.GetOrigNodeID(s.first.dest)
//                  << " c: " << s.second.second << std::endl;
//        auto path = shortestPaths.at(s.first);
//        std::cout << "key test success" <<std::endl;
//    }

    std::vector<FlowCost>::const_iterator it;
    #pragma omp parallel default(shared) private(it) num_threads(LOCAL_NUM_THREADS)
    {
    for (it = flowCost.begin(); it != flowCost.end(); ++it)
    {
        #pragma omp single nowait
        {
        auto fc = *it;
        string oldStartNodeStr;
        string oldEndNodeStr;

        //only valid if mcfNet is queried!
        auto mcfStartNode       = mcfNet.GetSourceNode(fc.intArc);
        auto mcfEndNode         = mcfNet.GetTargetNode(fc.intArc);
        auto mcfStartNodeIntID  = mcfNet.GetNodeID(mcfStartNode);
        auto mcfEndNodeIntID    = mcfNet.GetNodeID(mcfEndNode);

        // OD Matrix nodes
        string mcfStartNodeID    = mcfNet.GetOrigNodeID( mcfStartNode );
        string mcfEndNodeID      = mcfNet.GetOrigNodeID( mcfEndNode );

        try {
            // filter out dummy nodes because t hey cannot be resolved to shortest paths of ODMatrix
            if (mcfStartNodeID != "dummy" && mcfEndNodeID != "dummy")
            {
                //now query the OD net, because it has results od the ODMatrix Solver which was run with net
                auto odStartNode        = net.GetNodeFromOrigID(mcfStartNodeID);
                auto odEndNode          = net.GetNodeFromOrigID(mcfEndNodeID);
                auto odStartNodeIntID   = net.GetNodeID(odStartNode);
                auto odEndNodeIntID     = net.GetNodeID(odEndNode);

//                std::cout << "ShortestPath query of " <<odStartNodeIntID <<"->" <<odEndNodeIntID <<".." << std::endl;
                try
                {
                    #pragma omp critical
                    {
                        //build key for result map
                        ODPair resultKey {odStartNode, odEndNode} ;

                        auto path = shortestPaths.at(resultKey);

                        DistributionArc resultVal { path, fc.flow};

                        this->distribution.insert(make_pair(resultKey, resultVal));
                    }//omp critical
                }
                catch (std::out_of_range& ex) {
                    LOGGER::LogError("Pair of shortest path "+
                                     mcfStartNodeID + " - "+ mcfEndNodeID+
                                    " could not be looked up!");

                    std::cout << "int ids: "<< mcfStartNodeIntID << "->" << mcfEndNodeIntID
                              << " VS " << odStartNodeIntID << "->" << odEndNodeIntID
                              <<" flow: " << fc.flow << " cost: "<< fc.cost<< std::endl;
                }
                catch (std::exception& ex) {
                    LOGGER::LogError(ex.what() );
                }
            }
            else
            {
                LOGGER::LogWarning("Dummy: FromTo "+
                                    mcfStartNodeID + " - "+ mcfEndNodeID+
                                    " could not be looked up!");
            }
        }
        catch (exception& ex) {
            LOGGER::LogError("Something strange happened.");
            LOGGER::LogError(ex.what());
        }
        }//omp single
    } //for
    }//omp parallel
}




std::vector<netxpert::data::node_t>
 Transportation::GetOrigins() const {
    return this->originNodes;
}

void
 Transportation::SetOrigins(std::vector<netxpert::data::node_t>& origs) {
    this->originNodes = origs;
}

std::vector<netxpert::data::node_t>
 Transportation::GetDestinations() const {
    return this->destinationNodes;
}

void
 Transportation::SetDestinations(std::vector<netxpert::data::node_t>& dests) {
    this->destinationNodes = dests;
}

void
 Transportation::SetExtODMatrix(std::vector<ExtSPTreeArc> _extODMatrix) {
    this->extODMatrix = _extODMatrix;
}

std::map<ODPair, DistributionArc>
 Transportation::GetDistribution() const {
    return this->distribution;
}

std::vector<ExtDistributionArc>
 Transportation::GetExtDistribution() const {

    vector<ExtDistributionArc> distArcs;
    uint32_t counter = 0;
    for (auto& dist : this->distribution)
    {
        counter += 1;
        if (counter % 2500 == 0)
            LOGGER::LogInfo("Processed #" + to_string(counter) + " rows.");

        ODPair key = dist.first;
        DistributionArc val = dist.second;
        CompressedPath path = val.path;

        //get arc from key.origin and key.dest
        //TODO Check for duplicate arcs!
        auto arc = this->net->GetArcFromNodes(key.origin, key.dest);

        // only one arc
        unordered_set<string> arcIDs = this->net->GetOrigArcIDs(vector<netxpert::data::arc_t>
                                                                { arc });
        ExtArcID arcID = *arcIDs.begin();

        auto costPerPath = path.second;
        auto flow = val.flow;
        //TODO: get capacity per arc
//        auto cap = -1;

        string orig;
        string dest;
        try{
            orig = net->GetOrigNodeID(key.origin);
        }
        catch (exception& ex) {
            LOGGER::LogError(ex.what());
            orig = "dummy";
        }
        try{
            dest = net->GetOrigNodeID(key.dest);
        }
        catch (exception& ex) {
            LOGGER::LogError(ex.what());
            dest = "dummy";
        }
        //JSON output!
        distArcs.push_back( ExtDistributionArc {arcID, ExternalArc {orig, dest}, costPerPath, flow });
    }
    return distArcs;
}

std::string
 Transportation::GetJSONExtDistribution() const {
    return UTILS::SerializeObjectToJSON<vector<ExtDistributionArc>>(this->GetExtDistribution(),"distribution")+ "\n }";
}

std::string
 Transportation::GetSolverJSONResult() const {

    TransportationResult transpRes {this->optimum, this->GetExtDistribution() };
    return UTILS::SerializeObjectToJSON<TransportationResult>(transpRes, "result") + "\n }";
}

void
 Transportation::SetExtNodeSupply(vector<ExtNodeSupply> _nodeSupply) {
    this->extNodeSupply = _nodeSupply;
}

 netxpert::InternalNet* Transportation::GetNetwork() {
    return this->net;
}

void
 Transportation::SaveResults(const std::string& resultTableName, const ColumnMap& cmap) const {

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
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::TransportationSolver, true);
                writer->CommitCurrentTransaction();
                /*if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
                {*/
                auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
                qry = unique_ptr<SQLite::Statement> (sldbWriter.PrepareSaveResultArc(resultTableName, NetXpertSolver::TransportationSolver));
                //}
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
                writer->CreateNetXpertDB();
                writer->OpenNewTransaction();
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::TransportationSolver, true);
                writer->CommitCurrentTransaction();
            }
                break;
        }

        LOGGER::LogDebug("Writing Geometries..");
        writer->OpenNewTransaction();

		std::string arcIDs = "";
		std::unordered_set<string> totalArcIDs;
		std::map<ODPair, DistributionArc>::const_iterator it;

		if (cnfg.GeometryHandling == GEOMETRY_HANDLING::RealGeometry)
		{
			LOGGER::LogDebug("Preloading relevant geometries into Memory..");

			#pragma omp parallel default(shared) private(it) num_threads(LOCAL_NUM_THREADS)
			{
				//populate arcIDs
				for (it = this->distribution.begin(); it != this->distribution.end(); ++it)
				{
					#pragma omp single nowait
					{
						auto kv = *it;
//						ODPair key = kv.first;
						DistributionArc value = kv.second;
						auto route = value.path.first;
						std::unordered_set<std::string> arcIDlist;

						arcIDlist = this->net->GetOrigArcIDs(route);

						if (arcIDlist.size() > 0)
						{
							#pragma omp critical
							{
								for (std::string id : arcIDlist)
								{
									if (id != "dummy")
										totalArcIDs.insert(id);
								}
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
//				cout << "arcIDs: " << arcIDs << endl;
				DBHELPER::LoadGeometryToMem(cnfg.ArcsTableName, cmap, cnfg.ArcsGeomColumnName, arcIDs);
			}
			LOGGER::LogDebug("Done!");
		}

        int counter = 0;
		#pragma omp parallel shared(counter) private(it) num_threads(LOCAL_NUM_THREADS)
        {

        for (it = this->distribution.begin(); it != this->distribution.end(); ++it)
        {
            #pragma omp single nowait
            {
            auto dist = *it;

            counter += 1;
            if (counter % 2500 == 0)
                LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

            ODPair key = dist.first;
            DistributionArc val = dist.second;

            string arcIDs = "";
            auto arcs = val.path.first;
            auto cost = val.path.second;
            auto flow = val.flow;
            //TODO: get capacity per arc
            auto cap = -1;

            vector<ArcData> arcData;
            for (const auto& arc : arcs)
                arcData.push_back( this->net->GetArcData(arc) );

            for (ArcData& arcD : arcData)
            {
                if (arcD.extArcID.size() > 0)
                    arcIDs += arcD.extArcID += ",";
                //TODO: get capacity per arc
                //cap = arcD.capacity;
            }
            if (arcIDs.size() > 0)
                arcIDs.pop_back(); //trim last comma

            string orig = "";
            string dest = "";
            try{
                orig = this->net->GetOrigNodeID(key.origin);
            }
            catch (exception& ex){
                    orig = to_string(this->net->GetNodeID(key.origin));
            }

            try{
                dest = this->net->GetOrigNodeID(key.dest);
            }
            catch (exception& ex) {
                dest = to_string(this->net->GetNodeID(key.dest));
            }

//            std::cout << orig<< " " << dest<< " " << cost<< " " << cap<< " " << flow<< " " << std::endl;
//            cout << "arcIDs: " << arcIDs << endl;
            this->net->ProcessMCFResultArcsMem(orig, dest, cost, cap, flow, arcIDs, arcs, resultTableName, *writer, *qry);
            }//omp single
        }
        }//omp paralell

        writer->CommitCurrentTransaction();
        writer->CloseConnection();
        LOGGER::LogDebug("Done!");
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Transportation::SaveResults() - Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}

