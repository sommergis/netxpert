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

#include "mstree_simple.h"

using namespace netxpert::cnfg;
using namespace netxpert::io;
using namespace netxpert::utils;

netxpert::simple::MinimumSpanningTree::MinimumSpanningTree(std::string jsonCnfg)
{
    //Convert JSON Config to real Config Object
    NETXPERT_CNFG = UTILS::DeserializeJSONtoObject<netxpert::cnfg::Config>(jsonCnfg);
}

int netxpert::simple::MinimumSpanningTree::Solve()
{
    //local scope!
    using namespace std;
    using namespace netxpert;
    using namespace netxpert::data;

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

        LOGGER::LogInfo("Converting Data into internal network..");
        InternalNet net (arcsTable, cmap, cnfg);

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
const double
 netxpert::simple::MinimumSpanningTree::GetOptimum() const
{
    double result = 0;
    if (this->solver)
        result = this->solver->GetOptimum();
    return result;
}

std::string
 netxpert::simple::MinimumSpanningTree::GetMinimumSpanningTreeAsJSON()
{
  std::string result;
  if (this->solver) {
    std::vector<std::string> tmpRes  = this->GetMinimumSpanningTree();
    result = netxpert::utils::UTILS::SerializeObjectToJSON<std::vector<std::string>>(tmpRes, "mst");
  }
  return result;
}

std::vector<std::string>
 netxpert::simple::MinimumSpanningTree::GetMinimumSpanningTree()
{
  std::unordered_set<std::string> tmpRes = this->solver->GetOrigMinimumSpanningTree();
  std::vector<std::string> result (tmpRes.begin(), tmpRes.end());

  return result;
}
