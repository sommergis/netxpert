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

        mst.SaveResults(resultTableName, cmap);

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
