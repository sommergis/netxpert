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

#include "odmatrix_simple.h"

using namespace netxpert::cnfg;
using namespace netxpert::io;
using namespace netxpert::utils;

netxpert::simple::OriginDestinationMatrix::OriginDestinationMatrix(std::string jsonCnfg)
{
    //Convert JSON Config to real Config Object
    NETXPERT_CNFG = UTILS::DeserializeJSONtoObject<netxpert::cnfg::Config>(jsonCnfg);
}
int netxpert::simple::OriginDestinationMatrix::Solve()
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
        catch (std::exception& ex)
        {
            std::cout << "Error creating log file: " + cnfg.LogFileFullPath << std::endl;
            std::cout << ex.what() << std::endl;
        }

        InputArcs arcsTable;
        std::vector<NewNode> nodesTable;

        std::string arcsGeomColumnName = cnfg.ArcsGeomColumnName;

        std::string pathToSpatiaLiteDB = cnfg.NetXDBPath;
        std::string arcsTableName = cnfg.ArcsTableName;

        std::string nodesTableName = cnfg.NodesTableName;
        std::string nodesGeomColName = cnfg.NodesGeomColumnName;
        std::string resultTableName = cnfg.ResultTableName.empty() ? cnfg.ArcsTableName + "_odm" : cnfg.ResultTableName;
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

        LOGGER::LogInfo("Converting Data into internal network..");
        InternalNet net (arcsTable, cmap, cnfg);
        LOGGER::LogInfo("Done!");

        LOGGER::LogInfo("Loading Start nodes..");
        std::vector<std::pair<uint32_t, std::string>> startNodes = net.LoadStartNodes(nodesTable, cnfg.Threshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);
        LOGGER::LogInfo("Loading End nodes..");
        std::vector<std::pair<uint32_t, std::string>> endNodes = net.LoadEndNodes(nodesTable, cnfg.Threshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();

        // Solver
        solver = std::unique_ptr<netxpert::OriginDestinationMatrix> (new netxpert::OriginDestinationMatrix(cnfg));
        auto& odm = *solver;
        std::vector<netxpert::data::node_t> origs = {}; //newStartNodeID, newStartNodeID2};
        for (auto s : startNodes)
            origs.push_back(net.GetNodeFromID(s.first));

        odm.SetOrigins( origs );

        std::vector<netxpert::data::node_t> dests = {}; //newEndNodeID, newEndNodeID2}; //newEndNodeID}; // {}
        for (auto e : endNodes)
            dests.push_back(net.GetNodeFromID(e.first));

        odm.SetDestinations( dests );

        odm.Solve(net);

        LOGGER::LogInfo("Optimum: " + to_string(odm.GetOptimum()));
        LOGGER::LogInfo("Count of ODMatrix: " +to_string( odm.GetODMatrix().size() ) );

        odm.SaveResults(resultTableName, cmap);

        return 0; //OK
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError("OriginDestination_Simple::Solve() - Unexpected Error!");
        LOGGER::LogError(ex.what());
        return 1; //Not OK
    }
}


int netxpert::simple::OriginDestinationMatrix::Solve(bool doParallel)
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
        catch (std::exception& ex)
        {
            std::cout << "Error creating log file: " + cnfg.LogFileFullPath << std::endl;
            std::cout << ex.what() << std::endl;
        }

        InputArcs arcsTable;
        std::vector<NewNode> nodesTable;

        std::string arcsGeomColumnName = cnfg.ArcsGeomColumnName;

        std::string pathToSpatiaLiteDB = cnfg.NetXDBPath;
        std::string arcsTableName = cnfg.ArcsTableName;

        std::string nodesTableName = cnfg.NodesTableName;
        std::string nodesGeomColName = cnfg.NodesGeomColumnName;
        std::string resultTableName = cnfg.ResultTableName.empty() ? cnfg.ArcsTableName + "_odm" : cnfg.ResultTableName;
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
        std::vector<std::pair<uint32_t, std::string>> startNodes = net.LoadStartNodes(nodesTable, cnfg.Threshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);
        LOGGER::LogInfo("Loading End nodes..");
        std::vector<std::pair<uint32_t, std::string>> endNodes = net.LoadEndNodes(nodesTable, cnfg.Threshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();

        /*if (this->experimentalVersion)
        {
            solver2 = std::unique_ptr<netxpert::OriginDestinationMatrix2> (new netxpert::OriginDestinationMatrix2(cnfg));
            auto& odm = *solver2;
            std::vector<uint32_t> origs = {}; //newStartNodeID, newStartNodeID2};
            for (auto s : startNodes)
                origs.push_back(s.first);

            odm.SetOrigins( origs );

            std::vector<uint32_t> dests = {}; //newEndNodeID, newEndNodeID2}; //newEndNodeID}; // {}
            for (auto e : endNodes)
                dests.push_back(e.first);

            odm.SetDestinations( dests );

            odm.Solve(net);

            LOGGER::LogInfo("Optimum: " + to_string(odm.GetOptimum()));
            LOGGER::LogInfo("Count of ODMatrix: " +to_string( odm.GetODMatrix().size() ) );

            odm.SaveResults(resultTableName, cmap);

            return 0; //OK
        }

        else
        {*/
            solver = std::unique_ptr<netxpert::OriginDestinationMatrix> (new netxpert::OriginDestinationMatrix(cnfg));
            auto& odm = *solver;

            std::vector<netxpert::data::node_t> origs = {}; //newStartNodeID, newStartNodeID2};
            for (auto s : startNodes)
                origs.push_back(net.GetNodeFromID(s.first));

            odm.SetOrigins( origs );

            std::vector<netxpert::data::node_t> dests = {}; //newEndNodeID, newEndNodeID2}; //newEndNodeID}; // {}
            for (auto e : endNodes)
                dests.push_back(net.GetNodeFromID(e.first));

            odm.SetDestinations( dests );

            odm.Solve(net);

            LOGGER::LogInfo("Optimum: " + to_string(odm.GetOptimum()));
            LOGGER::LogInfo("Count of ODMatrix: " +to_string( odm.GetODMatrix().size() ) );

            odm.SaveResults(resultTableName, cmap);

            return 0; //OK
//        }
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError("OriginDestination_Simple::Solve() - Unexpected Error!");
        LOGGER::LogError(ex.what());
        return 1; //Not OK
    }
}

const double
 netxpert::simple::OriginDestinationMatrix::GetOptimum() const
{
    double result = 0;
    if (this->solver)
      result = this->solver->GetOptimum();

    return result;
}

std::string
 netxpert::simple::OriginDestinationMatrix::GetODMatrixAsJSON()
{
    std::string result;
    if (this->solver)
      result = this->solver->GetResultsAsJSON();

    return result;
}
std::vector<netxpert::data::extarcid_t>
 netxpert::simple::OriginDestinationMatrix::GetODMatrix()
{
    std::vector<netxpert::data::extarcid_t> result;
    //problem: resolve internal ODPair to external arcIDs

    return result;
}
