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

#include "test.hpp"

using namespace std;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::data;
using namespace netxpert::io;
using namespace netxpert::utils;

using namespace geos::geom;

namespace netxpert {
  namespace test {

    static void
     loadNodes(netxpert::data::InternalNet& net,
                 std::string arcsGeomColumnName,
                 std::string arcsTableName,
                 netxpert::data::ColumnMap& cmap)   {

      std::string nodeID = "1";
      LOGGER::LogInfo("Querying NodeID "+ nodeID + "..");
      auto node = net.GetNodeFromOrigID(nodeID);
      auto supply = net.GetNodeSupply(node);

      LOGGER::LogInfo("Original node ID of 1: " + net.GetOrigNodeID(node) );

      LOGGER::LogDebug("Testing LoadStart/EndNodes..");
      geos::geom::Coordinate coord = {703444, 5364720};
      string extNodeID = "1";
      //double supply = 5.0;
      NewNode startNode {extNodeID, coord, supply};
      NewNode endNode {extNodeID, geos::geom::Coordinate {703342, 5364710}, supply};

      auto startNodes = vector<NewNode> {startNode};
      auto endNodes   = vector<NewNode> {endNode};

      auto starts = net.LoadStartNodes(startNodes, 500, arcsTableName, arcsGeomColumnName, cmap, false);

      auto ends = net.LoadEndNodes(endNodes, 500, arcsTableName, arcsGeomColumnName, cmap, false);
    }
  } //namespace test
} //namespace netxpert

void netxpert::test::LemonNetworkConvert(Config& cnfg)
{
    try
    {
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
        string arcsGeomColumnName = cnfg.ArcsGeomColumnName; //"Geometry";

        string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString();
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString();

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;

        bool autoCleanNetwork = cnfg.CleanNetwork;

        ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

        //2. Load Network
        DBHELPER::OpenNewTransaction();
        arcsTable = DBHELPER::LoadNetworkFromDB(arcsTableName, cmap);
        nodesTable = DBHELPER::LoadNodesFromDB(nodesTableName, nodesGeomColName, cmap);
        LOGGER::LogInfo("Converting Data into internal network..");
        netxpert::data::InternalNet net (arcsTable, cmap, cnfg);
        LOGGER::LogInfo("Done!");

        loadNodes(net, arcsGeomColumnName, arcsTableName, cmap);
        LOGGER::LogDebug("Done!");

        LOGGER::LogDebug("Reset of Network..");
        net.Reset();
        LOGGER::LogDebug("Done!");

        loadNodes(net, arcsGeomColumnName, arcsTableName, cmap);
        LOGGER::LogDebug("Done!");

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();
    }
    catch (exception& ex)
    {
        LOGGER::LogError("LemonNetworkConvert: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}


void netxpert::test::TestNetworkBuilder(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "config") + "}";
	cout << s << endl;
	netxpert::simple::NetworkBuilder simpleSolver(s);
	simpleSolver.Build();
}

void netxpert::test::TestFileGDBWriter(Config& cnfg)
{
    try
    {
        string wkt = "MULTILINESTRING((1 2, 3 4), (5 6, 7 8, 9 10), (11 12, 13 14))";
        string orig = "1";
        string dest = "2";
        const string resultTblName = cnfg.ArcsTableName + "_net";
        const bool dropFirst = true;

        geos::io::WKTReader reader;
        auto geomPtr = reader.read(wkt);
        std::shared_ptr<geos::geom::MultiLineString> mlPtr (dynamic_cast<geos::geom::MultiLineString*>(geomPtr));

        FGDBWriter fgdb(cnfg);
        fgdb.CreateNetXpertDB();

        fgdb.CreateSolverResultTable(resultTblName, NetXpertSolver::UndefinedNetXpertSolver, dropFirst);
        fgdb.OpenNewTransaction();
        fgdb.SaveResultArc(orig, dest, 1.0, 99999.0, 1.0, *mlPtr, resultTblName);
        fgdb.CommitCurrentTransaction();
        fgdb.CloseConnection();

    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestFileGDBWriter: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}

void netxpert::test::TestSpatiaLiteWriter(Config& cnfg)
{
    try
    {
        string wkt = "MULTILINESTRING((1 2, 3 4), (5 6, 7 8, 9 10), (11 12, 13 14))";
        string orig = "1";
        string dest = "2";
        const string resultTblName = cnfg.ArcsTableName + "_net";
        const bool dropFirst = true;

        geos::io::WKTReader reader;
        auto geomPtr = reader.read(wkt);
        std::shared_ptr<geos::geom::MultiLineString> mlPtr (dynamic_cast<geos::geom::MultiLineString*>(geomPtr));

        SpatiaLiteWriter sldb( cnfg );
        sldb.CreateNetXpertDB();
        sldb.OpenNewTransaction();
        sldb.CreateSolverResultTable(resultTblName, NetXpertSolver::UndefinedNetXpertSolver, dropFirst);
        auto queryPtr = sldb.PrepareSaveResultArc(resultTblName, NetXpertSolver::UndefinedNetXpertSolver);
        //depointerization "on the fly"
        sldb.SaveResultArc(orig, dest, 1.0, 99999.0, 1.0, *mlPtr, resultTblName, *queryPtr);
        sldb.CommitCurrentTransaction();
        //delete queryPtr;

        sldb.CloseConnection();
        //delete mlPtr;
    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestSpatiaLiteWriter: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}
void netxpert::test::TestCreateRouteGeometries(Config& cnfg)
{
	using namespace std;

    try
    {
        string wkt1 = "LINESTRING(1 2, 3 4, 5 6, 7 8, 9 10)";
        string wkt2 = "LINESTRING(11 12, 13 14)";

        const string resultTblName = cnfg.ArcsTableName + "_test";
        const bool dropFirst = true;

        geos::io::WKTReader reader;
        auto geomPtr1 = reader.read(wkt1);
        auto geomPtr2 = reader.read(wkt2);

        SpatiaLiteWriter sldb( cnfg );
        sldb.OpenNewTransaction();
        sldb.CreateSolverResultTable (resultTblName, NetXpertSolver::UndefinedNetXpertSolver, dropFirst);

        vector<shared_ptr<Geometry>> segs;
        // {shared_ptr<Geometry>(geomPtr1), shared_ptr<Geometry>(geomPtr2) };
        vector<shared_ptr<Geometry>>::iterator it;
        segs.resize(2);
        it = segs.begin();
        cout << "vector init & resized.." << endl;

        /*it = segs.insert(it, shared_ptr<Geometry>(geomPtr1));
        segs.insert(it+1, shared_ptr<Geometry>(geomPtr2));*/

        auto mPtr = unique_ptr<MultiLineString>( DBHELPER::GEO_FACTORY->createMultiLineString(
                                                    {geomPtr1, geomPtr2 }) );

        //segs.insert(it, shared_ptr<Geometry>(geomPtr1));
        //segs.insert(it+1, shared_ptr<Geometry>(geomPtr2));
        //cout << "insert done "<< endl;

        sldb.MergeAndSaveResultArcs("1", "2", -1, -1, -1, cnfg.ArcsGeomColumnName, cnfg.ArcIDColumnName,
                                    cnfg.ArcsTableName, "1,2", *mPtr, resultTblName);

        sldb.CommitCurrentTransaction();
        sldb.CloseConnection();

    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestSpatiaLiteWriter: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}
void netxpert::test::TestAddNodes(Config& cnfg)
{
//	using namespace std;
//
//    try
//    {
//        //1. Config
//        if (!DBHELPER::IsInitialized)
//        {
//            DBHELPER::Initialize(cnfg);
//        }
//
//        try
//        {
//            if (!LOGGER::IsInitialized)
//            {
//                LOGGER::Initialize(cnfg);
//            }
//        }
//        catch (exception& ex)
//        {
//            cout << "Error creating log file: " + cnfg.LogFileFullPath << endl;
//            cout << ex.what() << endl;
//        }
//
//        InputArcs arcsTable;
//        InputNodes nodesTable;
//        string arcsGeomColumnName = cnfg.ArcsGeomColumnName; //"Geometry";
//
//        string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString();
//        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString();
//
//        string nodesTableName = cnfg.NodesTableName;
//        string nodesGeomColName = cnfg.NodesGeomColumnName;
//
//        bool autoCleanNetwork = cnfg.CleanNetwork;
//
//        ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
//                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
//                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };
//
//        bool withCapacity = false;
//        if (!cnfg.CapColumnName.empty())
//            withCapacity = true;
//
//        //2. Load Network
//        DBHELPER::OpenNewTransaction();
//
//        LOGGER::LogInfo("Loading Data from DB..!");
//        arcsTable = DBHELPER::LoadNetworkFromDB(arcsTableName, cmap);
//        //nodesTable = DBHELPER::LoadNodesFromDB(nodesTableName, cmap);
//        LOGGER::LogInfo("Done!");
//        Network net (arcsTable, cmap, cnfg);
//        LOGGER::LogInfo("Converting Data into internal network..");
//        net.ConvertInputNetwork(autoCleanNetwork);
//        LOGGER::LogInfo("Done!");
//
//        auto qry = DBHELPER::PrepareGetClosestArcQuery(arcsTableName, cnfg.ArcsGeomColumnName,
//                                            cmap, ArcIDColumnDataType::Number, withCapacity);
//
//        geos::geom::Coordinate coord = {703444, 5364720};
//        //LOGGER::LogDebug(to_string(coord.x) + " " + to_string(coord.y) );
//
//        /*ExtClosestArcAndPoint result = DBHELPER::GetClosestArcFromPoint(coord, cnfg.Threshold, *qry, withCapacity);
//
//        LOGGER::LogDebug("Closest Arc ID: " + result.extArcID);
//        LOGGER::LogDebug("Closest Point X: " + to_string(result.closestPoint.x));
//        LOGGER::LogDebug("Closest Point Y: " + to_string(result.closestPoint.y));
//        LOGGER::LogDebug("Closest Arc cost: " + to_string(result.cost));
//        LOGGER::LogDebug("Closest Arc cap: " + to_string(result.capacity));
//        LOGGER::LogDebug("Closest Arc geom: " + result.arcGeom->toString());
//
//        ExtFTNode key = {result.extFromNode, result.extToNode};
//        ArcData arcData = {result.extArcID, result.cost, result.capacity};
//        //shared_ptr<LineString> line = dynamic_pointer_cast<LineString>(result.arcGeom);
//        auto p = make_pair(key, arcData);
//
//        const Geometry& arc = *result.arcGeom;
//        SplittedArc sArc = net.GetSplittedClosestOldArcToPoint(coord, cnfg.Threshold, p, arc);
//
//        stringstream ss;
//        ss << "Splitted Closest New Arc is: " << endl
//           << "     fromNode " << sArc.ftNode.fromNode << endl
//           << "     toNode " << sArc.ftNode.toNode << endl
//
//           << sArc.arcGeom->toString() << endl;
//
//        cout << ss.str() << endl;*/
//
//        string extNodeID = "1";
//        double supply = 5.0;
//        NewNode startNode {extNodeID, coord, supply};
//        NewNode endNode {extNodeID, geos::geom::Coordinate {703342, 5364710}, supply};
//
//        uint32_t newEndNodeID = net.AddEndNode(startNode, cnfg.Threshold, *qry, false);
//        cout << "New End Node ID: " << newEndNodeID << endl;
//
//        newEndNodeID = net.AddEndNode(endNode, cnfg.Threshold, *qry, false);
//        cout << "New End Node ID: " << newEndNodeID << endl;
//
//        uint32_t newStartNodeID = net.AddStartNode(startNode, cnfg.Threshold, *qry, false);
//        cout << "New Start Node ID: " << newStartNodeID << endl;
//
//        newStartNodeID = net.AddStartNode(endNode, cnfg.Threshold, *qry, false);
//        cout << "New Start Node ID: " << newStartNodeID << endl;
//
//        cout << "Reset of Network.." << endl;
//        net.Reset();
//
//        newEndNodeID = net.AddEndNode(startNode, cnfg.Threshold, *qry, false);
//        cout << "New End Node ID: " << newEndNodeID << endl;
//
//        newEndNodeID = net.AddEndNode(endNode, cnfg.Threshold, *qry, false);
//        cout << "New End Node ID: " << newEndNodeID << endl;
//
//        newStartNodeID = net.AddStartNode(startNode, cnfg.Threshold, *qry, false);
//        cout << "New Start Node ID: " << newStartNodeID << endl;
//
//        newStartNodeID = net.AddStartNode(endNode, cnfg.Threshold, *qry, false);
//        cout << "New Start Node ID: " << newStartNodeID << endl;
//        //delete qry;
//
//        DBHELPER::CommitCurrentTransaction();
//        DBHELPER::CloseConnection();
//    }
//    catch (exception& ex)
//    {
//        LOGGER::LogError("AddStartNode: Unexpected Error!");
//        LOGGER::LogError(ex.what());
//    }
}

void netxpert::test::TestMST(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "config") + "}";
	cout << s << endl;
	netxpert::simple::MinimumSpanningTree simpleSolver(s);
	simpleSolver.Solve();
}

void netxpert::test::TestSPT(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "config") + "}";
	cout << s << endl;
	LOGGER::LogDebug("netxpert Version: "+ netxpert::Version());
	netxpert::simple::ShortestPathTree simpleSolver(s);
	simpleSolver.Solve();
}

void netxpert::test::TestODMatrix(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "config") + "}";
	cout << s << endl;
    LOGGER::LogDebug("netxpert Version: "+ netxpert::Version());
	netxpert::simple::OriginDestinationMatrix simpleSolver(s);
	simpleSolver.Solve(true);
}

void netxpert::test::TestTransportation(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "config") + "}";
	cout << s << endl;
    LOGGER::LogDebug("netxpert Version: "+ netxpert::Version());
	netxpert::simple::Transportation simpleSolver(s);
	simpleSolver.Solve();
}

void netxpert::test::TestMCF(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "config") + "}";
	cout << s << endl;
	LOGGER::LogDebug("netxpert Version: "+ netxpert::Version());
	netxpert::simple::MinCostFlow simpleSolver(s);
	simpleSolver.Solve();
}

//void netxpert::test::TestTransportationExt(Config& cnfg)
//{
//    try
//    {
//        //1. Config
//        if (!DBHELPER::IsInitialized)
//        {
//            DBHELPER::Initialize(cnfg);
//        }
//
//        try
//        {
//            if (!LOGGER::IsInitialized)
//            {
//                LOGGER::Initialize(cnfg);
//            }
//        }
//        catch (exception& ex)
//        {
//            cout << "Error creating log file: " + cnfg.LogFileFullPath << endl;
//            cout << ex.what() << endl;
//        }
//
//        InputArcs arcsTable;
//        vector<NewNode> nodesTable;
//
//        string arcsGeomColumnName = cnfg.ArcsGeomColumnName; //"Geometry";
//
//        string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString();
//        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString();
//
//        string nodesTableName = cnfg.NodesTableName;
//        string nodesGeomColName = cnfg.NodesGeomColumnName;
//        string resultTableName = cnfg.ArcsTableName + "_transpext";
//        bool dropFirst = true;
//
//        bool autoCleanNetwork = cnfg.CleanNetwork;
//
//        ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
//                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
//                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };
//
//        bool withCapacity = false;
//        if (!cnfg.CapColumnName.empty())
//            withCapacity = true;
//
//        //2. Load Network
//        DBHELPER::OpenNewTransaction();
//        LOGGER::LogInfo("Loading Data from DB..!");
//        arcsTable = DBHELPER::LoadNetworkFromDB(arcsTableName, cmap);
//        nodesTable = DBHELPER::LoadNodesFromDB(nodesTableName, cnfg.NodesGeomColumnName, cmap);
//        LOGGER::LogInfo("Done!");
//
//        DBHELPER::CommitCurrentTransaction();
//        DBHELPER::CloseConnection();
//
//        //input arcs to extODMatrix
//        std::vector<ExtSPTreeArc> extODMatrix;
//        for (InputArc& a : arcsTable)
//		{
//			extODMatrix.push_back(ExtSPTreeArc{ a.extArcID, ExternalArc{ a.extFromNode, a.extToNode }, a.cost });
//        }
//        //input nodes to extNodeSupply - no coords!
//        vector<ExtNodeSupply> extNodeSupply;
//        for (NewNode& node : nodesTable)
//        {
//            extNodeSupply.push_back( ExtNodeSupply {node.extNodeID, node.supply} );
//        }
//        //Transportation Solver
//        Transportation transp(cnfg);
//        //TransportationExt -> external setting of ODMatrix and nodesupply
//
//        ExtTransportationData input {extODMatrix, extNodeSupply};
//
//        transp.SetExtODMatrix(input.odm);
//        transp.SetExtNodeSupply(input.supply);
//
//        transp.Solve();
//
//        Network net = transp.GetNetwork();
//
//        LOGGER::LogInfo("Done!");
//        LOGGER::LogInfo("Optimum: " + to_string(transp.GetOptimum()) );
//        LOGGER::LogInfo("Count of Distributions: " + to_string(transp.GetDistribution().size()) );
//
//        map<ODPair, DistributionArc> result = transp.GetDistribution();
//
//        unique_ptr<DBWriter> writer;
//        switch (cnfg.ResultDBType)
//        {
//            case RESULT_DB_TYPE::SpatiaLiteDB:
//            {
//                writer = unique_ptr<DBWriter> (new SpatiaLiteWriter(cnfg)) ;
//            }
//                break;
//            case RESULT_DB_TYPE::ESRI_FileGDB:
//            {
//                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
//            }
//                break;
//        }
//        writer->CreateNetXpertDB();
//        writer->OpenNewTransaction();
//        writer->CreateSolverResultTable(resultTableName, NetXpertSolver::TransportationSolver, true);
//        writer->CommitCurrentTransaction();
//        LOGGER::LogDebug("Writing Geometries..");
//        writer->OpenNewTransaction();
//
//        vector<ExtDistributionArc> distArcs;
//
//        uint32_t counter = 0;
//        for (auto& dist : result)
//        {
//            counter += 1;
//            if (counter % 2500 == 0)
//                LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");
//
//            ODPair key = dist.first;
//            DistributionArc val = dist.second;
//            CompressedPath path = val.path;
//            vector<uint32_t> ends = val.path.first;
//            double costPerPath = path.second;
//            double flow = val.flow;
//            //TODO: get capacity per arc
//            double cap = -1;
//
//            //cout << key.origin << "_>" << key.dest << endl;
//            // only one arc
//            unordered_set<string> arcIDs = net.GetOrigArcIDs(vector<InternalArc>
//                                                                        { InternalArc  {key.origin, key.dest} },
//                                                                        transp.IsDirected);
//            ExtArcID arcID = *arcIDs.begin();
//
//            //auto route = transp.UncompressRoute(key.origin, ends);
//            //cout << route.size() << endl;
//            //cout << "From: " << key.origin << " To: " << key.dest << " flows: " << flow << " ,cost: " << costPerPath << endl;
//
//            string orig;
//            string dest;
//            try{
//                orig = net.GetOrigNodeID(key.origin);
//            }
//            catch (exception& ex) {
//                LOGGER::LogError(ex.what());
//            }
//            try{
//                dest = net.GetOrigNodeID(key.dest);
//            }
//            catch (exception& ex) {
//                LOGGER::LogError(ex.what());
//            }
//
//            //JSON output!
//            distArcs.push_back( ExtDistributionArc {arcID, ExternalArc {orig, dest}, costPerPath, flow });
//
//            /*net.BuildTotalRouteGeometry(orig, dest, costPerPath, -1, flow, "", route,
//                                        resultTableName, *writer);*/
//
//        }
//        writer->CommitCurrentTransaction();
//        writer->CloseConnection();
//
//        TransportationResult transpRes {transp.GetOptimum(), distArcs};
//        cout << UTILS::SerializeObjectToJSON<TransportationResult>(transpRes, "result") + "\n }"<< endl;
//        LOGGER::LogDebug("Done!");
//
//    }
//    catch (exception& ex)
//    {
//        LOGGER::LogError("TestTransportationExt: Unexpected Error!");
//        LOGGER::LogError(ex.what());
//    }
//}


#if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)
void netxpert::test::TestSPTCH(Config& cnfg) {
    try {
        //1. Config
        if (!DBHELPER::IsInitialized){
            DBHELPER::Initialize(cnfg);
        }

        try {
            if (!LOGGER::IsInitialized) {
                LOGGER::Initialize(cnfg);
            }
        }
        catch (exception& ex) {
            cout << "Error creating log file: " + cnfg.LogFileFullPath << endl;
            cout << ex.what() << endl;
        }

        InputArcs arcsTable;
        vector<NewNode> nodesTable;
        string arcsGeomColumnName = cnfg.ArcsGeomColumnName; //"Geometry";

        string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString();
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString();

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;

        string resultTableName = cnfg.ArcsTableName + "_spt_ch";
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

        //CH
        net.ComputeContraction(100);
//        net.ExportContractedNetwork(cnfg.ArcsTableName + "-ch-export");

        LOGGER::LogInfo("Loading Start nodes..");
        vector<pair<uint32_t, string>> startNodes = net.LoadStartNodes(nodesTable, cnfg.Threshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);
        vector<pair<uint32_t, string>> endNodes;
        if (!cnfg.SPTAllDests) {
            LOGGER::LogInfo("Loading End nodes..");
            endNodes = net.LoadEndNodes(nodesTable, cnfg.Threshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);
        }

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();

        //CH
        net.ExportContractedNetwork(cnfg.ArcsTableName + "-ch-loadnodes-export");

        // Solver
        ShortestPathTree spt(cnfg);

        spt.SetOrigin(startNodes.at(0).first);
        vector<uint32_t> dests = {};// newEndNodeID, newEndNodeID2}; //newEndNodeID}; // {}

//        if (!cnfg.SPTAllDests) {
//            for (auto d : endNodes)
//                dests.push_back(d.first);
//        }
        // only 1-1 search for CH
        dests.push_back(endNodes.at(0).first);
        spt.SetDestinations( dests );

        spt.Solve(net);

        LOGGER::LogInfo("Optimum: " + to_string(spt.GetOptimum()) );
        LOGGER::LogInfo("Count of SPT: " +to_string( spt.GetShortestPaths().size() ) );

        auto kvSPS = spt.GetShortestPaths();

//        unique_ptr<DBWriter> writer;
//        switch (cnfg.ResultDBType)
//        {
//            case RESULT_DB_TYPE::SpatiaLiteDB:
//            {
//                writer = unique_ptr<DBWriter> (new SpatiaLiteWriter(cnfg)) ;
//            }
//                break;
//            case RESULT_DB_TYPE::ESRI_FileGDB:
//            {
//                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
//            }
//                break;
//        }
//        writer->OpenNewTransaction();
//        writer->CreateNetXpertDB();
//        writer->CreateSolverResultTable(resultTableName, true);
//        writer->CommitCurrentTransaction();
//        LOGGER::LogDebug("Writing Geometries..");
//        writer->OpenNewTransaction();
//        int counter = 0;
//        for (auto kv : kvSPS)
//        {
//            counter += 1;
//            string arcIDs = "";
//            ODPair key = kv.first;
//            CompressedPath value = kv.second;
//            vector<uint32_t> ends = value.first;
//            double costPerPath = value.second;
//
//            auto route = spt.UncompressRoute(key.origin, ends);
//
//            vector<string> arcIDlist = net.GetOriginalArcIDs(route, cnfg.IsDirected);
//
//            if (arcIDlist.size() > 0)
//            {
//                for (string& id : arcIDlist)
//                    arcIDs += id += ",";
//                arcIDs.pop_back(); //trim last comma
//            }
//            string orig;
//            string dest;
//            try{
//                orig = net.GetOriginalStartOrEndNodeID(key.origin);
//            }
//            catch (exception& ex) {
//                orig = net.GetOriginalNodeID(key.origin);
//            }
//            try{
//                dest = net.GetOriginalStartOrEndNodeID(key.dest);
//            }
//            catch (exception& ex) {
//                dest = net.GetOriginalNodeID(key.dest);
//            }
//            net.ProcessResultArcs(orig, dest, costPerPath, -1, -1, arcIDs, route, resultTableName, *writer);
//        }
//        writer->CommitCurrentTransaction();
//        writer->CloseConnection();
//        LOGGER::LogDebug("Done!");
    }
    catch (exception& ex) {
        LOGGER::LogError("TestSPTCH: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}

#endif // NETX_ENABLE_CONTRACTION_HIERARCHIES


