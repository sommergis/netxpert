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

#include "isolines_simple.h"

using namespace netxpert::cnfg;
using namespace netxpert::io;
using namespace netxpert::utils;

netxpert::simple::Isolines::Isolines(std::string jsonCnfg)
{
    //Convert JSON Config to real Config Object
    this->NETXPERT_CNFG = UTILS::DeserializeJSONtoObject<netxpert::cnfg::Config>(jsonCnfg);
}

double netxpert::simple::Isolines::GetOptimum()
{
    return this->optimum;
}

std::string netxpert::simple::Isolines::GetShortestPathsAsJSON()
{
    std::string result;
    /*if (this->solver)
        result = this->solver->GetShortestPathsAsJSON();*/
    return result;
}
std::vector<netxpert::data::ExtSPTreeArc> netxpert::simple::Isolines::GetShortestPaths()
{
    std::vector<netxpert::data::ExtSPTreeArc> result;
    /*if (this->solver)
        result = this->solver->GetShortestPaths();*/
    return result;
}

int netxpert::simple::Isolines::Solve()
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
        vector<pair<uint32_t, string>> startNodes = net.LoadStartNodes(nodesTable, cnfg.Treshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();

        //solve
        this->solver = unique_ptr<netxpert::ShortestPathTree> (new netxpert::ShortestPathTree(cnfg));
        auto& spt = *solver;

        //for start in starts
        vector<uint32_t> origs = {}; //newStartNodeID, newStartNodeID2};
        for (auto s : startNodes)
            origs.push_back(s.first);

        vector<uint32_t>::const_iterator it;
        this->optimum = 0;

        for (it = origs.begin(); it != origs.end(); it++)
        {
            auto start = *it;
            spt.SetOrigin(start);
            vector<uint32_t> dests = {}; //null -> all dests

            spt.SetDestinations( dests );
            spt.Solve(net);
            this->optimum += spt.GetOptimum();
			auto localSPTs = spt.GetShortestPaths();

			//TODO: from unordered_map<ODPair, CompressedPath> to vector<ExtSPTreeArc>

			//this->totalSPTs.insert();
            //TODO: cut off
            spt.SaveResults(cnfg.ResultTableName, cmap);
        }

        LOGGER::LogInfo("Optimum: " + to_string(this->optimum));
        LOGGER::LogInfo("Count of ShortestPaths: " +to_string( this->totalSPTs.size() ) );

        return 0; // OK
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Isolines_Simple::Solve() - Unexpected Error!");
        LOGGER::LogError(ex.what());
        return 1;
    }
}

