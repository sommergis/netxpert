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
    if (this->net)
        delete this->net;
}

void
 Transportation::Solve() {

//    if (this->extODMatrix.size() == 0 || this->extNodeSupply.size() == 0)
//        throw std::runtime_error("OD-Matrix and node supply must be filled in Transportation Solver!");
//
//    //arcData from ODMatrix
//    InputArcs arcs;
//    for (ExtSPTreeArc& v : extODMatrix)
//    {
//        ExtArcID key = v.extArcID;
//        ExternalArc arc = v.extArc;
//        double cost = v.cost;
//        double cap = DOUBLE_INFINITY; // TRANSPORTATION Problem, no caps
//        string fromNode = arc.extFromNode;
//        string toNode = arc.extToNode;
//        string oneway = "";
//        arcs.push_back( InputArc {key, fromNode, toNode, cost, cap, oneway } );
//    }
//    Config cnfg = NETXPERT_CNFG;
//    ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
//                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
//                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };
//
//    //construct nodeSupply from external nodeSupply
//    InputNodes nodes;
//    for (ExtNodeSupply& v : extNodeSupply)
//    {
//        ExtNodeID key = v.extNodeID;
//        double supply = v.supply;
//        nodes.push_back( InputNode {key, supply });
//    }
//
//    //construct network from external ODMatrix
//    //TODO: checkme (pointer resource dealloc)
//    //problem line, because this->net seems not to be persistent anymore
//    // when used outside this scope
//
//    this->net = new InternalNet (arcs, nodes, cmap, cnfg);
//    InternalNet net = *this->net;
//
//    //Check Balancing of Transportation Problem Instance and
//    //transform the instance if needed is already done in
//    //parent class method MinCostFlow::Solve()
//
//    //Call MCF Solver of parent class
//    MinCostFlow::Solve(net);
//
//    //Go back from Origin->Dest: flow,cost to real path with a list of start and end nodes of the ODMatrix
//    //1. Translate back to original node ids
//    unordered_map<IntNodeID, ExtNodeID> startNodeMap;
//    unordered_map<IntNodeID, ExtNodeID> endNodeMap;
//    for (auto& n : net.GetNodeSupplies())
//    {
//        IntNodeID key = n.first;
//        NodeSupply ns = n.second;
//        if (ns.supply > 0 || ns.supply == 0) //start node
//        {
//            //cout<< "sm: inserting " << key << ": "<< ns.supply << endl;
//            startNodeMap.insert(make_pair(key, net.GetOriginalNodeID(key)));
//        }
//        if (ns.supply < 0 || ns.supply == 0) //end node
//        {
//            //cout<< "em: inserting " << key << ": "<< ns.supply << endl;
//            endNodeMap.insert(make_pair(key, net.GetOriginalNodeID(key)));
//        }
//    }
//    //2. Search for the ODpairs in ODMatrix Solver result with the original IDs of the
//    // Min Cost Flow Solver result
//    vector<FlowCost> flowCost = MinCostFlow::GetMinCostFlow();
//    for (auto& fc : flowCost)
//    {
//        // Here the original node id can be an arbitrary string, because it was set externally
//        // so we cannot translate back to uint_fast32_ts as in Solve(net)
//        string oldStartNodeStr;
//        string oldEndNodeStr;
//        try
//        {
//            //TODO: dummys
//            // At the moment they will not be given back, because their arc cannot be resolved
//            oldStartNodeStr = startNodeMap.at(fc.intArc.fromNode);
//            oldEndNodeStr = endNodeMap.at(fc.intArc.toNode);
//
//            if (oldStartNodeStr != "dummy" && oldEndNodeStr != "dummy")
//            {
//                //build key for result map
//                ODPair resultKey {fc.intArc.fromNode, fc.intArc.toNode};
//                // flow and cost
//                // search for a match of startNode -> endNode in FlowCost,
//                // take the first occurence
//                // as flow and cost
//                auto data = getFlowCostData(flowCost, resultKey);
//
//                // there is no path, because we do not have the values from the OD solver
//                DistributionArc resultVal { CompressedPath { make_pair(vector<uint32_t> {}, data.second) }, data.first };
//
//                this->distribution.insert(make_pair(resultKey, resultVal));
//            }
//            /*else
//            {
//                LOGGER::LogInfo("Dummys!");
//            }*/
//        }
//        catch (exception& ex)
//        {
//            LOGGER::LogError("Something strange happened - maybe a key error. ");
//            LOGGER::LogError(ex.what());
//        }
//    }
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
        ODPair key = kv.first;
        auto cost = kv.second;
        auto cap = DOUBLE_INFINITY; // TRANSPORTATION Problem, no caps
        string fromNode = to_string(net.GetNodeID(key.origin));
        string toNode   = to_string(net.GetNodeID(key.dest));

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

    // Node supply has been set already in the given Network (parameter for Solve())

    // Add nodes data only, if reached through ODMatrix Solver
    InputNodes nodes;
    auto* supplyMap = net.GetSupplyMap();
    auto nodesIter = net.GetNodesIter();
    for (; nodesIter != lemon::INVALID; ++nodesIter) {
        auto node       = nodesIter;
        supply_t supply = net.GetNodeSupply(node);
        // precondition: only reached nodes from ODMatrix solver shall be
        // processed
        if (odKeys.count(node) > 0)
            nodes.push_back( InputNode {to_string(net.GetNodeID(node)), supply });
    }

    //construct network from ODMatrix
    InternalNet newNet(arcs, cmap, cnfg, nodes);

    //Check Balancing of Transportation Problem Instance and
    //transform the instance if needed is already done in
    //parent class method MinCostFlow::Solve()

    //Call MCF Solver of parent class
    MinCostFlow::Solve(newNet);

    //Go back from Origin->Dest: flow,cost to real path with a list of start and end nodes of the ODMatrix
    //1. Translate back to original node ids
    map<netxpert::data::node_t, ExtNodeID> startNodeMap;
    map<netxpert::data::node_t, ExtNodeID> endNodeMap;

    nodesIter = newNet.GetNodesIter();
    supplyMap = newNet.GetSupplyMap();
    for (; nodesIter != lemon::INVALID; ++nodesIter)
    {
        auto node       = nodesIter;
        supply_t supply = newNet.GetNodeSupply(node);
        auto extNodeID  = newNet.GetOrigNodeID(node);

        if (supply > 0 || supply == 0) //start node
        {
            //cout<< "sm: inserting " << key << ": "<< ns.supply << endl;
            // We know that the original node id is actually an uint because it was transformed in the ODMatrix solver		            }
            // but dummys are possible because of balancing of the TP
            if (extNodeID != "dummy")
            {
                IntNodeID origNodeID = static_cast<IntNodeID>(std::stoul(extNodeID, nullptr, 0));
                startNodeMap.insert(make_pair(newNet.GetNodeFromID(origNodeID), extNodeID));
            }
        }
        if (supply < 0 || supply == 0) //end node
        {
            //cout<< "em: inserting " << key << ": "<< ns.supply << endl;
            // We know that the original node id is actually an uint because it was transformed in the ODMatrix solver
            // but dummys are possible because of balancing of the TP
            if (extNodeID != "dummy")
            {
                IntNodeID origNodeID = static_cast<IntNodeID>(std::stoul(extNodeID, nullptr, 0));
                endNodeMap.insert(make_pair(newNet.GetNodeFromID(origNodeID), extNodeID));
            }
        }
    }

    //debug
    /*for (auto& s : startNodeMap)
        cout << s.first << "_>" << s.second << endl;
    for (auto& e : endNodeMap)
        cout << e.first << "_>" << e.second << endl;*/

    //2. Search for the ODpairs in ODMatrix Solver result with the original IDs of the
    // Min Cost Flow Solver result
    vector<FlowCost> flowCost = MinCostFlow::GetMinCostFlow();
    map<ODPair, CompressedPath> shortestPaths = ODsolver.GetShortestPaths();
    for (auto& fc : flowCost)
    {
        // We know that the original node id is actually an IntNodeID because it was transformed
        // in the ODMatrix solver
        string oldStartNodeStr;
        string oldEndNodeStr;
        IntNodeID oldStartNode = 0;
        IntNodeID oldEndNode = 0;

        //cout << newNet.GetOriginalNodeID(fc.intArc.fromNode) << "->" <<newNet.GetOriginalNodeID(fc.intArc.toNode) <<endl;
        //cout << fc.intArc.fromNode << "->" <<fc.intArc.toNode << " f: "<< fc.flow << " c: "<<fc.cost <<endl;
        try
        {
            oldStartNodeStr = newNet.GetOrigNodeID(newNet.GetSourceNode(fc.intArc));
            oldEndNodeStr   = newNet.GetOrigNodeID(newNet.GetTargetNode(fc.intArc));

            if (oldStartNodeStr.size() > 0 && oldStartNodeStr != "dummy")
                oldStartNode = static_cast<IntNodeID>(std::stoul(oldStartNodeStr, nullptr, 0));
            if (oldEndNodeStr.size() > 0 && oldEndNodeStr != "dummy")
                oldEndNode = static_cast<IntNodeID>(std::stoul(oldEndNodeStr, nullptr, 0));

            if (oldStartNode > 0 && oldEndNode > 0)
            {
                //build key for result map
                ODPair resultKey { newNet.GetNodeFromID(oldStartNode), newNet.GetNodeFromID(oldEndNode) };
                // flow and cost
                // search for a match of startNode -> endNode in FlowCost,
                // take the first occurence
                // as flow and cost
                auto data = getFlowCostData(flowCost, ODPair { newNet.GetSourceNode(fc.intArc),
                                                               newNet.GetTargetNode(fc.intArc) });
                //cout << "flow: " << data.first << " :" << data.second << endl;
                auto path = shortestPaths.at( resultKey );

                DistributionArc resultVal { path, data.first };

                this->distribution.insert(make_pair(resultKey, resultVal));
            }
            /*else
            {
                LOGGER::LogWarning("Dummy: FromTo "+
                                    oldStartNodeStr + " - "+ oldEndNodeStr+
                                    " could not be looked up!");
            }*/
        }
        catch (exception& ex)
        {
            LOGGER::LogError("Something strange happened - maybe a key error. ");
            LOGGER::LogError(ex.what());
        }
    }
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
        auto cap = -1;

        string orig;
        string dest;
        try{
            orig = net->GetOrigNodeID(key.origin);
        }
        catch (exception& ex) {
            LOGGER::LogError(ex.what());
        }
        try{
            dest = net->GetOrigNodeID(key.dest);
        }
        catch (exception& ex) {
            LOGGER::LogError(ex.what());
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

std::pair<netxpert::data::flow_t,netxpert::data::cost_t>
 Transportation::getFlowCostData(const vector<FlowCost>& fc, const ODPair& key) const {

    pair<netxpert::data::flow_t,netxpert::data::cost_t> result;
    for (const auto& f : fc) {
        const ODPair vKey {this->net->GetSourceNode(f.intArc), this->net->GetTargetNode(f.intArc)};
        if ( vKey == key )
        {
            result = make_pair(f.flow, f.cost);
            break;
        }
    }
    return result;
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
						ODPair key = kv.first;
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
                //cout << arcD.extArcID << endl;
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

