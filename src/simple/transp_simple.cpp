#include "transp_simple.h"

netxpert::simple::Transportation::Transportation(std::string jsonCnfg)
{
    //Convert JSON Config to real Config Object
    NETXPERT_CNFG = netxpert::UTILS::DeserializeJSONtoObject<netxpert::Config>(jsonCnfg);
}
int netxpert::simple::Transportation::Solve()
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
        string resultTableName = cnfg.ResultTableName.empty() ? cnfg.ArcsTableName + "_transp" : cnfg.ResultTableName;
        bool dropFirst = true;

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
        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();

        //Transportation Solver
        solver = unique_ptr<netxpert::Transportation> (new netxpert::Transportation(cnfg));
        auto& transp = *solver;

        vector<unsigned int> origs;
        for (auto& p : startNodes)
            origs.push_back(p.first);

        vector<unsigned int> dests;
        for (auto& p : endNodes)
            dests.push_back(p.first);

        transp.SetOrigins(origs);
        transp.SetDestinations(dests);

        transp.Solve(net);
        LOGGER::LogInfo("Done!");

        LOGGER::LogInfo("Optimum: " + to_string(transp.GetOptimum()) );
        unordered_map<ODPair, DistributionArc> result = transp.GetDistribution();
        LOGGER::LogInfo("Count of Distributions: " + to_string(result.size()) );

        unique_ptr<DBWriter> writer;
        unique_ptr<SQLite::Statement> qry; //is null in case of ESRI FileGDB
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
        writer->OpenNewTransaction();
        int counter = 0;
        for (auto& dist : result)
        {
            counter += 1;
            if (counter % 2500 == 0)
                LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

            ODPair key = dist.first;
            DistributionArc val = dist.second;

            string arcIDs = "";
            CompressedPath path = val.path;
            double cost = path.second;
            double flow = val.flow;
            //TODO: get capacity per arc
            double cap = -1;

            vector<InternalArc> arcs = transp.UncompressRoute(key.origin, path.first);
            vector<ArcData> arcData = net.GetOriginalArcData(arcs, cnfg.IsDirected);

            for (ArcData& arcD : arcData)
            {
                //cout << arcD.extArcID << endl;
                arcIDs += arcD.extArcID += ",";
                //TODO: get capacity per arc
                //cap = arcD.capacity;
            }
            if (arcIDs.size() > 0)
                arcIDs.pop_back(); //trim last comma

            string orig;
            string dest;
            try{
                orig = net.GetOriginalStartOrEndNodeID(key.origin);
            }
            catch (exception& ex) {
                orig = net.GetOriginalNodeID(key.origin);
            }
            try{
                dest = net.GetOriginalStartOrEndNodeID(key.dest);
            }
            catch (exception& ex) {
                dest = net.GetOriginalNodeID(key.dest);
            }
            net.ProcessResultArcs(orig, dest, cost, cap, flow, arcIDs, arcs, resultTableName, *writer, *qry);
        }
        writer->CommitCurrentTransaction();
        writer->CloseConnection();
        LOGGER::LogDebug("Done!");
        return 0; //OK
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Transportation_Simple::Solve() - Unexpected Error!");
        LOGGER::LogError(ex.what());
        return 1; //Not OK
    }
}
double netxpert::simple::Transportation::GetOptimum()
{
    double result = 0;
    if (this->solver)
        result = this->solver->GetOptimum();
    return result;
}
std::string netxpert::simple::Transportation::GetDistributionAsJSON()
{
    string result;
    if (this->solver)
        result = this->solver->GetJSONExtDistribution();
    return result;
}

std::vector<netxpert::ExtDistributionArc> netxpert::simple::Transportation::GetDistribution()
{
    std::vector<netxpert::ExtDistributionArc> result;
    if (this->solver)
        result = this->solver->GetExtDistribution();
    return result;
}
