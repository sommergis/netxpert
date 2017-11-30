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

#include "mcfp_simple.h"

using namespace netxpert::cnfg;
using namespace netxpert::io;
using namespace netxpert::utils;

netxpert::simple::MinCostFlow::MinCostFlow(std::string jsonCnfg)
{
    //Convert JSON Config to real Config Object
    NETXPERT_CNFG = UTILS::DeserializeJSONtoObject<netxpert::cnfg::Config>(jsonCnfg);
}

int netxpert::simple::MinCostFlow::Solve()
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
        vector<NewNode> nodesTable;
        string arcsGeomColumnName = cnfg.ArcsGeomColumnName;

        string pathToSpatiaLiteDB = cnfg.NetXDBPath;
        string arcsTableName = cnfg.ArcsTableName;

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;
        string resultTableName = cnfg.ResultTableName.empty() ? cnfg.ArcsTableName + "_mcf" : cnfg.ResultTableName;
        //bool dropFirst = true;

        bool autoCleanNetwork = cnfg.CleanNetwork;

//		LOGGER::LogInfo("Using # " + to_string(LOCAL_NUM_THREADS) + " threads.");

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

        LOGGER::LogInfo("Converting Data into internal network..");
        InternalNet net (arcsTable, cmap, cnfg);
        LOGGER::LogInfo("Done!");

        LOGGER::LogInfo("Loading Start nodes..");
        vector<pair<uint32_t, string>> startNodes = net.LoadStartNodes(nodesTable, cnfg.Threshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);
        LOGGER::LogInfo("Loading End nodes..");
        vector<pair<uint32_t, string>> endNodes = net.LoadEndNodes(nodesTable, cnfg.Threshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);

        LOGGER::LogInfo("Done!");

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();
        LOGGER::LogInfo("Done!");

        //MCF Solver
        solver = unique_ptr<netxpert::MinCostFlow>(new netxpert::MinCostFlow (cnfg));
        auto& mcf = *solver;
        mcf.Solve(net);
        LOGGER::LogInfo("Done!");

        vector<FlowCost> mcfResult = mcf.GetMinCostFlow();

        LOGGER::LogInfo("Optimum: " + to_string(mcf.GetOptimum()));
        LOGGER::LogInfo("Count of MCF: " + to_string(mcfResult.size()) );

        mcf.SaveResults(resultTableName, cmap);

        return 0; //OK
    }
    catch (exception& ex)
    {
        LOGGER::LogError("MinCostFlow_Simple::Solve() - Unexpected Error!");
        LOGGER::LogError(ex.what());
        return 1; //Not OK
    }
}

const double
 netxpert::simple::MinCostFlow::GetOptimum() const
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

std::vector<netxpert::data::FlowCost> netxpert::simple::MinCostFlow::GetMinimumCostFlow()
{
    std::vector<netxpert::data::FlowCost> result;
    if (this->solver)
        result = this->solver->GetMinCostFlow();
    return result;
}
