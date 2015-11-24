#include "mstree_simple.h"

netxpert::simple::MinimumSpanningTree::MinimumSpanningTree(std::string jsonCnfg)
{
    //Convert JSON Config to real Config Object
    NETXPERT_CNFG = netxpert::UTILS::DeserializeJSONtoObject<netxpert::Config>(jsonCnfg);
}

int netxpert::simple::MinimumSpanningTree::Solve()
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
        string arcsGeomColumnName = cnfg.ArcsGeomColumnName;

        string pathToSpatiaLiteDB = cnfg.NetXDBPath;
        string arcsTableName = cnfg.ArcsTableName;

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;
        string resultTableName = cnfg.ResultTableName.empty() ? cnfg.ArcsTableName + "_mst" : cnfg.ResultTableName;
        //bool dropFirst = true;

        bool autoCleanNetwork = cnfg.CleanNetwork;

        ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

        //2. Load Network
        DBHELPER::OpenNewTransaction();
        LOGGER::LogInfo("Loading Data from DB..!");
        arcsTable = DBHELPER::LoadNetworkFromDB(arcsTableName, cmap);
        LOGGER::LogInfo("Done!");
        Network net (arcsTable, cmap, cnfg);

        LOGGER::LogInfo("Converting Data into internal network..");
        net.ConvertInputNetwork(autoCleanNetwork);

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();
        LOGGER::LogInfo("Done!");

        //MST Solver
        solver = unique_ptr<netxpert::MinimumSpanningTree>(new netxpert::MinimumSpanningTree (cnfg));
        auto& mst = *solver;
        mst.Solve(net);
        LOGGER::LogInfo("Done!");

        LOGGER::LogInfo("Optimum: " + to_string(mst.GetOptimum()));
        LOGGER::LogInfo("Count of MST: " + to_string(mst.GetMinimumSpanningTree().size()) );

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
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::MinSpanningTreeSolver, true);
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
                writer->CreateSolverResultTable(resultTableName, NetXpertSolver::MinSpanningTreeSolver, true);
                writer->CommitCurrentTransaction();
                writer->CloseConnection();
            }
                break;
        }

        LOGGER::LogDebug("Writing Geometries..");

        //Processing and Saving Results are handled within net.ProcessResultArcs()

        string arcIDs;
        unordered_set<string> arcIDlist = net.GetOriginalArcIDs(mst.GetMinimumSpanningTree(), cnfg.IsDirected);
        for (string id : arcIDlist)
        {
            // 13-14 min on 840000 arcs
            //arcIDs = arcIDs + id + ","; //+= is c++ concat operator!
            arcIDs += id += ","; //optimized! 0.2 seconds on 840000 arcs
        }
        arcIDs.pop_back(); //trim last comma

        net.ProcessResultArcs("", "", -1, -1, -1, arcIDs, resultTableName);

        LOGGER::LogDebug("Done!");
        return 0; //OK
    }
    catch (exception& ex)
    {
        LOGGER::LogError("MinimumSpanningTree_Simple::Solve() - Unexpected Error!");
        LOGGER::LogError(ex.what());
        return 1; //Not OK
    }
}
double netxpert::simple::MinimumSpanningTree::GetOptimum()
{
    double result = 0;
    if (this->solver)
        result = this->solver->GetOptimum();
    return result;
}

std::string netxpert::simple::MinimumSpanningTree::GetMinimumSpanningTreeAsJSON()
{
    string result;
    /*if (this->solver)
        result = this->solver->GetMinimumSpanningTreeAsJSON();
    */
    return result;
}

std::vector<netxpert::ExternalArc> netxpert::simple::MinimumSpanningTree::GetMinimumSpanningTree()
{
    std::vector<netxpert::ExternalArc> result;
    /*if (this->solver)
        result = this->solver->GetMinimumSpanningTree();*/
    return result;
}
