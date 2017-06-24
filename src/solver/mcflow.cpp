#include "mcflow.h"

using namespace std;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::data;
using namespace netxpert::io;
using namespace netxpert::core;
using namespace netxpert::utils;

MinCostFlow::MinCostFlow(Config& cnfg)
{
    //ctor
    LOGGER::LogInfo("MinimumCostFlow Solver instantiated");
    NETXPERT_CNFG = cnfg;
    algorithm = cnfg.McfAlgorithm;
    solverStatus = MCFSolverStatus::MCFUnSolved;
    IsDirected = true; //TODO CHECK always true?
}

void MinCostFlow::Solve(string net){}

void MinCostFlow::Solve(Network& net)
{
    //TODO: Check for leaks
    this->net = &net;

    //Check Balancing of Min Cost Flow Instance
    auto type = net.GetMinCostFlowInstanceType();
    LOGGER::LogInfo("Type of Min Cost Flow Instance: "+ to_string(type));

    //And transform the instance if needed
    net.TransformUnbalancedMCF(type);
    LOGGER::LogInfo("Transformed Min Cost Flow Problem done.");
    try {
        solve(net);
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Exception solving MCF Problem.");
    }
}

void MinCostFlow::SaveResults(const std::string& resultTableName, const ColumnMap& cmap) const
{
    try
    {
        Config cnfg = this->NETXPERT_CNFG;

        unique_ptr<DBWriter> writer;
        unique_ptr<SQLite::Statement> qry;
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
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::MinCostFlowSolver, true);
                writer->CommitCurrentTransaction();
                /*if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
                {*/
                auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
                qry = unique_ptr<SQLite::Statement> (sldbWriter.PrepareSaveResultArc(resultTableName, NetXpertSolver::MinCostFlowSolver));
                //}
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
                writer->CreateNetXpertDB();
                writer->OpenNewTransaction();
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::MinCostFlowSolver, true);
                writer->CommitCurrentTransaction();
            }
                break;
        }

        LOGGER::LogDebug("Writing Geometries..");
        writer->OpenNewTransaction();

        std::string arcIDs = "";
        std::unordered_set<string> totalArcIDs;
        std::vector<FlowCost>::iterator it;
        vector<FlowCost> mcfResult = this->GetMinCostFlow();

		if (cnfg.GeometryHandling == GEOMETRY_HANDLING::RealGeometry)
		{
			LOGGER::LogDebug("Preloading relevant geometries into Memory..");

			#pragma omp parallel default(shared) private(it) num_threads(LOCAL_NUM_THREADS)
			{
				//populate arcIDs
				for (it = mcfResult.begin(); it != mcfResult.end(); ++it)
				{
					#pragma omp single nowait
					{
						auto arcFlow = *it;
						InternalArc key = arcFlow.intArc;
						double cost = arcFlow.cost;
						double flow = arcFlow.flow;
						vector<InternalArc> arc{ key };
						string id = "";

						vector<ArcData> arcData = this->net->GetOriginalArcData(arc, cnfg.IsDirected);
						// is only one arc
						if (arcData.size() > 0)
						{
							ArcData arcD = *arcData.begin();
							id = arcD.extArcID;
							#pragma omp critical
							{
								if (id != "dummy")
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
        for (it = mcfResult.begin(); it != mcfResult.end(); ++it)
        {
            #pragma omp single nowait
            {
            auto arcFlow = *it;

            counter += 1;
            if (counter % 2500 == 0)
                LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

            string arcIDs = "";
            InternalArc key = arcFlow.intArc;
            double cost = arcFlow.cost;
            double flow = arcFlow.flow;
            double cap = -1;
            vector<InternalArc> arc { key };

            // cout << key.fromNode << "->" << key.toNode << endl;
            //TODO:
            //works only on non-splitted arcs!
            vector<ArcData> arcData = this->net->GetOriginalArcData(arc, cnfg.IsDirected);
            // is only one arc
            if (arcData.size() > 0)
            {
                ArcData arcD = *arcData.begin();
                arcIDs = arcD.extArcID;
                cap = arcD.capacity;
                cout << "cap: " <<cap << endl;
            }

            string orig;
            string dest;
            try{
                orig = this->net->GetOriginalStartOrEndNodeID(key.fromNode);
            }
            catch (exception& ex) {
                orig = this->net->GetOriginalNodeID(key.fromNode);
            }
            try{
                dest = this->net->GetOriginalStartOrEndNodeID(key.toNode);
            }
            catch (exception& ex) {
                dest = this->net->GetOriginalNodeID(key.toNode);
            }

            if (orig != "dummy" && dest != "dummy")
                this->net->ProcessMCFResultArcsMem(orig, dest, cost, cap, flow, arcIDs, arc, resultTableName, *writer, *qry);
            else
                LOGGER::LogInfo("Dummy! orig: "+orig+", dest: "+ dest+", cost: "+to_string(cost)+ ", cap: "+
                                                to_string(cap) + ", flow: " +to_string(flow));
            }//omp single
        }
        }//omp paralell
        writer->CommitCurrentTransaction();
        writer->CloseConnection();
        LOGGER::LogDebug("Done!");
    }
    catch (exception& ex)
    {
        LOGGER::LogError("MinCostFlow::SaveResults() - Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}

MCFAlgorithm MinCostFlow::GetAlgorithm() const
{
    return algorithm;
}

void MinCostFlow::SetAlgorithm(MCFAlgorithm mcfAlgorithm)
{
    algorithm = mcfAlgorithm;
}

double MinCostFlow::GetOptimum() const
{
    return optimum;
}
vector<FlowCost> MinCostFlow::GetMinCostFlow() const
{
    return flowCost;
}

//private
void MinCostFlow::solve (Network& net)
{
    //vector<long> ign = { 0, -1, (long)4294967295, (long)4261281277 };
    // 0 is a valid ignore value for linux!
    //TODO check in windows
    vector<long> ign = { -1, (long)4294967295, (long)4261281277 };

    vector<FlowCost> result;

    vector<uint32_t> sNds;
    vector<uint32_t> eNds;
    vector<double> supply;
    vector<double> costs;
    vector<double> caps;
    uint32_t nmx = net.GetMaxNodeCount();
    uint32_t mmx = net.GetMaxArcCount();

    try
    {
        switch (algorithm)
        {
            case MCFAlgorithm::NetworkSimplex_LEMON:
                mcf = shared_ptr<IMinCostFlow>(new NetworkSimplex());
                break;
            default:
                mcf = shared_ptr<IMinCostFlow>(new NetworkSimplex());
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
    if (!validateNetworkData( net ))
        //throw new InvalidValueException(string.Format("Problem data does not fit the {0} Solver!", this.ToString()));
        throw;

    LOGGER::LogDebug("Arcs: " + to_string(net.GetMaxArcCount() ));
    LOGGER::LogDebug("Nodes: "+ to_string(net.GetMaxNodeCount() ));

    //Read the network
    try
    {
        convertInternalNetworkToSolverData(net, sNds, eNds, supply, caps, costs);

        int srcCount = 0;
        int transshipCount = 0;
        int sinkCount = 0;
        for (auto& s : supply)
        {
            if (s > 0)
                srcCount += 1;
            if (s == 0)
                transshipCount += 1;
            if (s < 0)
                sinkCount += 1;
        }

        LOGGER::LogDebug("Sources: "+ to_string(srcCount));
        LOGGER::LogDebug("Transshipment nodes: "+ to_string(transshipCount));
        LOGGER::LogDebug("Sinks: "+ to_string(sinkCount));
        LOGGER::LogDebug("Solving..");

        //vector::data() returns direct pointer to data
        mcf->LoadNet( static_cast<uint32_t>( net.GetMaxNodeCount() ), static_cast<uint32_t>( net.GetMaxArcCount()),
                        static_cast<uint32_t>( net.GetMaxNodeCount() ),static_cast<uint32_t>( net.GetMaxArcCount() ),
                        caps.data(), costs.data(), supply.data(), sNds.data(), eNds.data());
    }
    catch (exception& ex)
    {
        LOGGER::LogFatal( ex.what() );
    }

    try
    {
        mcf->SolveMCF();
    }
    catch (exception& ex)
    {
        //throw new ApplicationException(ex.Message, ex.InnerException);
    }
    solverStatus = static_cast<MCFSolverStatus>(mcf->MCFGetStatus());

    if (solverStatus == MCFSolverStatus::MCFOK)
    {
        try {
            //Check Solutions - if there is a failure an exception is thrown!
            mcf->CheckPSol(); //Primal Solution
        }
        catch (exception& ex) {
            LOGGER::LogError(ex.what());
        }
        try {
            mcf->CheckDSol(); //Dual Solution
        }
        catch (exception& ex) {
            LOGGER::LogWarning(ex.what());
        }

        optimum = mcf->MCFGetFO();

        vector<double> cost_flow;
        cost_flow.resize(mmx, 0);

        //fill data
        mcf->MCFGetX(cost_flow.data()); //data() gets raw pointer
        //cout << "mcf: cost_flow.size(): "<< cost_flow.size()<< endl;
        for (int i = 0; i < cost_flow.size(); i++)
        {
            uint32_t startNode = sNds[i];
            uint32_t endNode = eNds[i];
            double flow = cost_flow[i];
            double cost = costs[i];
            //cout << "s: " << startNode << " e: " <<endNode<<" f: "<<flow << " c: "<<cost << endl;
            if (flow > 0 && flow != ign[0] && flow != ign[1])
            {
                //double[] flowCost = { flow, cost };
                FlowCost fc{ InternalArc {startNode, endNode}, flow, cost};
                result.push_back(fc);
            }
        }
    }
    else
    {
        string ex = "MCF Solver Status not OK! Solverstatus: " + to_string(solverStatus);
        LOGGER::LogError(ex);
        throw std::runtime_error(ex);
    }
    this->flowCost = result;
}

bool MinCostFlow::validateNetworkData(Network& net)
{
    bool valid = false;

    valid = true;
    return valid;
}

void MinCostFlow::convertInternalNetworkToSolverData(Network& net, vector<uint32_t>& sNds,
                                                    vector<uint32_t>& eNds, vector<double>& supply,
                                                    vector<double>& caps, vector<double>& costs)
{
    // Die Größe der Arrays müssen passen (ob 0 drin steht ist egal, sonst gibts später bei Dispose
    // das böse Erwachen in Form eines Heap Corruption errors bzw. einer System Access Violation

    Arcs arcs = net.GetInternalArcData();
    vector<InternalArc> keys;
    for(Arcs::iterator it = arcs.begin(); it != arcs.end(); ++it) {
      keys.push_back(it->first);
    }
    // Auf die Richtung achten
    // --> doppelter Input der Kanten notwendig bei undirected
    if (this->IsDirected)
    {
        sNds.resize(keys.size());
        eNds.resize(keys.size());
        //cout << "size of arcs: " << keys.size() << endl;
        for (int i = 0; i < keys.size(); i++)
        {
            sNds[i] = keys[i].fromNode;
            eNds[i] = keys[i].toNode;
        }

        costs.resize(keys.size(), 0); //Größe muss passen!
        caps.resize(keys.size(), DOUBLE_INFINITY);  //Größe muss passen!
        for (int i = 0; i < keys.size(); i++)
        {
            ArcData oldArcData;
            if (arcs.count(keys[i]) > 0)
                oldArcData = arcs.at(keys[i]);
            costs[i] = oldArcData.cost;
            caps[i] = oldArcData.capacity;
        }
    }
    else //both directions!
    {
        sNds.resize(keys.size()*2);
        eNds.resize(keys.size()*2);
        //cout << "size of arcs: " << keys.size() << endl;
        for (int i = 0; i < keys.size(); i++)
        {
            sNds[i] = keys[i].fromNode;
            eNds[i] = keys[i].toNode;
            //undirected
            sNds[i + keys.size()] = keys[i].toNode;
            eNds[i + keys.size()] = keys[i].fromNode;
        }

        costs.resize(keys.size()*2, 0); //Größe muss passen!
        caps.resize(keys.size()*2, DOUBLE_INFINITY);  //Größe muss passen!
        for (int i = 0; i < keys.size(); i++)
        {
            ArcData oldArcData;
            if (arcs.count(keys[i]) > 0)
                oldArcData = arcs.at(keys[i]);
            costs[i] = oldArcData.cost;
            caps[i] = oldArcData.capacity;
            costs[i+keys.size()] = oldArcData.cost;
            caps[i+keys.size()] = oldArcData.capacity;
        }
    }
    //supply is direction independent
    supply.resize( net.GetMaxNodeCount(), 0 ); //Größe muss passen!
    //cout << "supply vector size: "<< supply.size() << endl;
    //cout << "net supply size: " << net.GetNodeSupplies().size() <<endl;
    for (auto item : net.GetNodeSupplies() )
    {
        uint32_t key = item.first;
        NodeSupply value = item.second;
        // key is 1-based thus -1 for index
        // only care for real supply and demand values
        // transshipment nodes (=0) are already present in the array (because of resize())
        supply[key - 1] = value.supply;
        //cout << "ns - " << key << ": " << value.supply << endl;
    }
    //cout << "ready converting data" << endl;
}
