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

#include "transp_simple.h"

using namespace netxpert::cnfg;
using namespace netxpert::io;
using namespace netxpert::utils;

netxpert::simple::Transportation::Transportation(std::string jsonCnfg)
{
    //Convert JSON Config to real Config Object
    NETXPERT_CNFG = UTILS::DeserializeJSONtoObject<netxpert::cnfg::Config>(jsonCnfg);
}
int netxpert::simple::Transportation::Solve()
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
        string resultTableName = cnfg.ResultTableName.empty() ? cnfg.ArcsTableName + "_transp" : cnfg.ResultTableName;
        bool dropFirst = true;

        bool autoCleanNetwork = cnfg.CleanNetwork;

        ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

        bool withCapacity = false;
        if (!cnfg.CapColumnName.empty())
            withCapacity = true;

//		LOGGER::LogInfo("Using # " + to_string(LOCAL_NUM_THREADS) + " threads.");

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

        //Transportation Solver
        solver = unique_ptr<netxpert::Transportation> (new netxpert::Transportation(cnfg));
        auto& transp = *solver;

        /*
        vector<uint32_t> origs;
        for (auto& p : startNodes)
            origs.push_back(p.first);

        vector<uint32_t> dests;
        for (auto& p : endNodes)
            dests.push_back(p.first);*/


        vector<node_t> origs;
        for (auto o : startNodes)
            origs.push_back(net.GetNodeFromID(o.first));

        vector<node_t> dests;
        for (auto d : endNodes)
            dests.push_back(net.GetNodeFromID(d.first));

        transp.SetOrigins(origs);
        transp.SetDestinations(dests);

        transp.Solve(net);
        LOGGER::LogInfo("Done!");

        LOGGER::LogInfo("Optimum: " + to_string(transp.GetOptimum()) );
        map<ODPair, DistributionArc> result = transp.GetDistribution();
        LOGGER::LogInfo("Count of Distributions: " + to_string(result.size()) );

        transp.SaveResults(resultTableName, cmap);

        return 0; //OK
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Transportation_Simple::Solve() - Unexpected Error!");
        LOGGER::LogError(ex.what());
        return 1; //Not OK
    }
}
const double
 netxpert::simple::Transportation::GetOptimum() const
{
    double result = 0;
    if (this->solver)
        result = this->solver->GetOptimum();
    return result;
}

std::string
 netxpert::simple::Transportation::GetDistributionAsJSON()
{
    std::string result;
    if (this->solver)
        result = this->solver->GetJSONExtDistribution();
    return result;
}

std::vector<netxpert::data::ExtDistributionArc>
 netxpert::simple::Transportation::GetDistribution()
{
    std::vector<netxpert::data::ExtDistributionArc> result;
    if (this->solver)
        result = this->solver->GetExtDistribution();
    return result;
}
