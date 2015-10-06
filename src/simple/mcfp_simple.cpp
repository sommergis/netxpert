#include "mcfp_simple.h"

netxpert::simple::MinCostFlow::MinCostFlow(std::string jsonCnfg)
{
    //Convert JSON Config to real Config Object
    NETXPERT_CNFG = netxpert::UTILS::DeserializeJSONtoObject<netxpert::Config>(jsonCnfg);
}

int netxpert::simple::MinCostFlow::Solve()
{
    using namespace netxpert; //local scope!

    try
    {
        Config cnfg = NETXPERT_CNFG;

        //1. Config
        if (!DBHELPER::IsInitialized)
        {
            DBHELPER::Initialize(cnfg);
        }

        try
        {
            if (!LOGGER::IsInitialized)
            {
                LOGGER::Initialize(cnfg);
            }
        }
        catch (exception& ex)
        {
            cout << "Error creating log file: " + cnfg.LogFileFullPath << endl;
            cout << ex.what() << endl;
        }

        InputArcs arcsTable;
        vector<NewNode> nodesTable;
        string arcsGeomColumnName = cnfg.ArcsGeomColumnName;

        string pathToSpatiaLiteDB = cnfg.NetXDBPath;
        string arcsTableName = cnfg.ArcsTableName;

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;
        string resultTableName = cnfg.ResultTableName.empty() ? cnfg.ArcsTableName + "_mcf" : cnfg.ResultTableName;
        //bool dropFirst = true;

        bool autoCleanNetwork = cnfg.CleanNetwork;

        ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

        bool withCapacity = false;
        if (!cnfg.CapColumnName.empty())
            withCapacity = true;

        //2. Load Network
        DBHELPER::OpenNewTransaction();
        LOGGER::LogInfo("Loading Data from DB..!");
        arcsTable = DBHELPER::LoadNetworkFromDB(arcsTableName, cmap);
        nodesTable = DBHELPER::LoadNodesFromDB(nodesTableName, cnfg.NodesGeomColumnName, cmap);
        LOGGER::LogInfo("Done!");

        Network net (arcsTable, cmap, cnfg);

        LOGGER::LogInfo("Converting Data into internal network..");
        net.ConvertInputNetwork(autoCleanNetwork);
        LOGGER::LogInfo("Done!");

        LOGGER::LogInfo("Loading Start nodes..");
        vector<pair<unsigned int, string>> startNodes = net.LoadStartNodes(nodesTable, cnfg.Treshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);
        LOGGER::LogInfo("Loading End nodes..");
        vector<pair<unsigned int, string>> endNodes = net.LoadEndNodes(nodesTable, cnfg.Treshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);

        LOGGER::LogInfo("Done!");
        LOGGER::LogInfo("Converting Data into internal network..");

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();
        LOGGER::LogInfo("Done!");

        //MST Solver
        solver = unique_ptr<netxpert::MinCostFlow>(new netxpert::MinCostFlow (cnfg));
        auto& mcf = *solver;
        mcf.Solve(net);
        LOGGER::LogInfo("Done!");

        vector<FlowCost> mcfResult = mcf.GetMinCostFlow();

        LOGGER::LogInfo("Optimum: " + to_string(mcf.GetOptimum()));
        LOGGER::LogInfo("Count of MCF: " + to_string(mcfResult.size()) );

        unique_ptr<DBWriter> writer;
        unique_ptr<SQLite::Statement> qry;
        switch (cnfg.ResultDBType)
        {
            case RESULT_DB_TYPE::SpatiaLiteDB:
            {
                if (NETXPERT_CNFG.ResultDBPath == NETXPERT_CNFG.NetXDBPath)
                {
                    //Override result DB Path with original netXpert DB path
                    writer = unique_ptr<DBWriter>(new SpatiaLiteWriter(cnfg, NETXPERT_CNFG.NetXDBPath));
                }
                else
				{
                    writer = unique_ptr<DBWriter>(new SpatiaLiteWriter(cnfg));
				}
                writer->CreateNetXpertDB(); //create before preparing query
                writer->OpenNewTransaction();
                writer->CreateSolverResultTable(resultTableName, true);
                writer->CommitCurrentTransaction();
                /*if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
                {*/
                auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
                qry = unique_ptr<SQLite::Statement> (sldbWriter.PrepareSaveResultArc(resultTableName));
                //}
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
                writer->CreateNetXpertDB();
                writer->OpenNewTransaction();
                writer->CreateSolverResultTable(resultTableName, true);
                writer->CommitCurrentTransaction();
            }
                break;
        }

        LOGGER::LogDebug("Writing Geometries..");
        //writer->OpenNewTransaction();
        //Processing and Saving Results are handled within net.ProcessResultArcs()

        string arcIDs;
        int counter = 0;
        for (FlowCost& arcFlow : mcfResult)
        {
            counter += 1;
            if (counter % 2500 == 0)
                LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

            string arcIDs = "";
            InternalArc key = arcFlow.intArc;
            double cost = arcFlow.cost;
            double flow = arcFlow.flow;
            double cap = -1;
            vector<InternalArc> arc { key };

            cout << key.fromNode << "->" << key.toNode << endl;

            vector<ArcData> arcData = net.GetOriginalArcData(arc, cnfg.IsDirected);
            // is only one arc
            if (arcData.size() > 0)
            {
                ArcData arcD = *arcData.begin();
                arcIDs = arcD.extArcID;
                cap = arcD.capacity;
            }

            string orig;
            string dest;
            try{
                orig = net.GetOriginalStartOrEndNodeID(key.fromNode);
            }
            catch (exception& ex) {
                orig = net.GetOriginalNodeID(key.fromNode);
            }
            try{
                dest = net.GetOriginalStartOrEndNodeID(key.toNode);
            }
            catch (exception& ex) {
                dest = net.GetOriginalNodeID(key.toNode);
            }

            if (orig != "dummy" && dest != "dummy")
                net.ProcessResultArcs(orig, dest, cost, cap, flow, arcIDs, arc, resultTableName, *writer, *qry);
            else
                LOGGER::LogInfo("Dummy! orig: "+orig+", dest: "+ dest+", cost: "+to_string(cost)+ ", cap: "+
                                                to_string(cap) + ", flow: " +to_string(flow));
        }

        LOGGER::LogDebug("Done!");

        return 0; //OK
    }
    catch (exception& ex)
    {
        LOGGER::LogError("MinCostFlow_Simple::Solve() - Unexpected Error!");
        LOGGER::LogError(ex.what());
        return 1; //Not OK
    }
}

double netxpert::simple::MinCostFlow::GetOptimum()
{
    double result = 0;
    if (this->solver)
        result = this->solver->GetOptimum();
    return result;
}

std::string netxpert::simple::MinCostFlow::GetMinimumCostFlowAsJSON()
{
    std::string result;

    return result;
}

std::vector<netxpert::FlowCost> netxpert::simple::MinCostFlow::GetMinimumCostFlow()
{
    std::vector<netxpert::FlowCost>  result;

    return result;
}
