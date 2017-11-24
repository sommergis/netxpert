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

#include "sptree_simple.h"

using namespace netxpert::cnfg;
using namespace netxpert::io;
using namespace netxpert::utils;

netxpert::simple::ShortestPathTree::ShortestPathTree(std::string jsonCnfg)
{
    //Convert JSON Config to real Config Object
    this->NETXPERT_CNFG = UTILS::DeserializeJSONtoObject<netxpert::cnfg::Config>(jsonCnfg);
}

int netxpert::simple::ShortestPathTree::Solve()
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

        string resultTableName = cnfg.ResultTableName.empty() ? cnfg.ArcsTableName + "_spt" : cnfg.ResultTableName;
        bool autoCleanNetwork = cnfg.CleanNetwork;

        ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

        bool withCapacity = false;
        if (!cnfg.CapColumnName.empty())
            withCapacity = true;

//      LOGGER::LogInfo("Using # " + to_string(LOCAL_NUM_THREADS) + " threads.");

        //2. Load Network
        DBHELPER::OpenNewTransaction();
        LOGGER::LogInfo("Loading Data from DB..!");
        arcsTable = DBHELPER::LoadNetworkFromDB(arcsTableName, cmap);
        nodesTable = DBHELPER::LoadNodesFromDB(nodesTableName, cnfg.NodesGeomColumnName, cmap);

        LOGGER::LogInfo("Done!");

        LOGGER::LogInfo("Converting Data into internal network..");
        InternalNet net (arcsTable, cmap, cnfg, netxpert::data::InputNodes{}, autoCleanNetwork);
        LOGGER::LogInfo("Done!");

//        net.PrintGraph();

        LOGGER::LogInfo("Loading Start nodes..");
        vector<pair<uint32_t, string>> startNodes = net.LoadStartNodes(nodesTable, cnfg.Treshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);
//        net.PrintGraph();

        vector<pair<uint32_t, string>> endNodes;
        if (!cnfg.SPTAllDests) {
            LOGGER::LogInfo("Loading End nodes..");
            endNodes = net.LoadEndNodes(nodesTable, cnfg.Treshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);
        }
//        net.PrintGraph();

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();

        //solve
        this->solver = unique_ptr<netxpert::ShortestPathTree> (new netxpert::ShortestPathTree(cnfg));
        auto& spt = *solver;

        spt.SetOrigin(net.GetNodeFromID(startNodes.at(0).first)); //spt has only 1 start!
        vector<netxpert::data::node_t> dests = {};// newEndNodeID, newEndNodeID2}; //newEndNodeID}; // {}

        if (!cnfg.SPTAllDests)
        {
            for (auto d : endNodes)
                dests.push_back(net.GetNodeFromID(d.first));
        }
        spt.SetDestinations( dests );

        spt.Solve(net);

        LOGGER::LogInfo("Optimum: " + to_string(spt.GetOptimum()));
        LOGGER::LogInfo("Count of ShortestPaths: " +to_string( spt.GetShortestPaths().size() ) );

//        spt.SaveResults(resultTableName, cmap);

        //test
        std::cout << spt.GetResultsAsJSON() << std::endl;

        return 0; // OK
    }
    catch (exception& ex)
    {
        LOGGER::LogError("ShortestPathTree_Simple::Solve() - Unexpected Error!");
        LOGGER::LogError(ex.what());
        return 1;
    }
}

const double
netxpert::simple::ShortestPathTree::GetOptimum() const {
    double result = 0;
    if (this->solver)
        result = this->solver->GetOptimum();
    return result;
}
std::string netxpert::simple::ShortestPathTree::GetShortestPathsAsJSON()
{
    std::string result;
    if (this->solver)
        result = this->solver->GetResultsAsJSON();
    return result;
}
std::vector<netxpert::data::ExtSPTreeArc> netxpert::simple::ShortestPathTree::GetShortestPaths()
{
    std::vector<netxpert::data::ExtSPTreeArc> result;
    /*if (this->solver)
        result = this->solver->GetShortestPaths();*/
    return result;
}
