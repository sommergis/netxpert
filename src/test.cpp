#include "test.h"

using namespace std;
using namespace netxpert;

void netxpert::Test::NetworkConvert(Config& cnfg)
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
        InputNodes nodesTable;
        string arcsGeomColumnName = cnfg.ArcsGeomColumnName; //"Geometry";

        string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString(); //@"C:\data\TRANSPRT_40.sqlite";
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"***REMOVED***_LINE_edges";

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;

        bool autoCleanNetwork = cnfg.CleanNetwork;

        ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

        //2. Load Network
        DBHELPER::OpenNewTransaction();
        arcsTable = DBHELPER::LoadNetworkFromDB(arcsTableName, cmap);
        Network net (arcsTable, cmap, cnfg);
        LOGGER::LogInfo("Converting Data into internal network..");
        net.ConvertInputNetwork(autoCleanNetwork);
        LOGGER::LogInfo("Done!");

        LOGGER::LogInfo("Original node ID of 1: " + net.GetOriginalNodeID(1) );

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();
    }
    catch (exception& ex)
    {
        LOGGER::LogError("NetworkConvert: Unerwarteter Fehler!");
        LOGGER::LogError(ex.what());
    }
}

void netxpert::Test::TestNetworkBuilder(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "c") + "}";
	cout << s << endl;
	netxpert::simple::NetworkBuilder simpleSolver(s);
	simpleSolver.Build();
}

void netxpert::Test::TestFileGDBWriter(Config& cnfg)
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

void netxpert::Test::TestSpatiaLiteWriter(Config& cnfg)
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
        auto queryPtr = sldb.PrepareSaveResultArc(resultTblName);
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
void netxpert::Test::TestCreateRouteGeometries(Config& cnfg)
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

        sldb.MergeAndSaveResultArcs("orig", "dest", 0.0, -1, -1, cnfg.ArcsGeomColumnName, cnfg.ArcIDColumnName,
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
void netxpert::Test::TestAddNodes(Config& cnfg)
{
	using namespace std;

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
        InputNodes nodesTable;
        string arcsGeomColumnName = cnfg.ArcsGeomColumnName; //"Geometry";

        string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString(); //@"C:\data\TRANSPRT_40.sqlite";
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"***REMOVED***_LINE_edges";

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;

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
        //nodesTable = DBHELPER::LoadNodesFromDB(nodesTableName, cmap);
        LOGGER::LogInfo("Done!");
        Network net (arcsTable, cmap, cnfg);
        LOGGER::LogInfo("Converting Data into internal network..");
        net.ConvertInputNetwork(autoCleanNetwork);
        LOGGER::LogInfo("Done!");

        auto qry = DBHELPER::PrepareGetClosestArcQuery(arcsTableName, cnfg.ArcsGeomColumnName,
                                            cmap, ArcIDColumnDataType::Number, withCapacity);

        geos::geom::Coordinate coord = {703444, 5364720};
        //LOGGER::LogDebug(to_string(coord.x) + " " + to_string(coord.y) );

        /*ExtClosestArcAndPoint result = DBHELPER::GetClosestArcFromPoint(coord, cnfg.Treshold, *qry, withCapacity);

        LOGGER::LogDebug("Closest Arc ID: " + result.extArcID);
        LOGGER::LogDebug("Closest Point X: " + to_string(result.closestPoint.x));
        LOGGER::LogDebug("Closest Point Y: " + to_string(result.closestPoint.y));
        LOGGER::LogDebug("Closest Arc cost: " + to_string(result.cost));
        LOGGER::LogDebug("Closest Arc cap: " + to_string(result.capacity));
        LOGGER::LogDebug("Closest Arc geom: " + result.arcGeom->toString());

        ExtFTNode key = {result.extFromNode, result.extToNode};
        ArcData arcData = {result.extArcID, result.cost, result.capacity};
        //shared_ptr<LineString> line = dynamic_pointer_cast<LineString>(result.arcGeom);
        auto p = make_pair(key, arcData);

        const Geometry& arc = *result.arcGeom;
        SplittedArc sArc = net.GetSplittedClosestOldArcToPoint(coord, cnfg.Treshold, p, arc);

        stringstream ss;
        ss << "Splitted Closest New Arc is: " << endl
           << "     fromNode " << sArc.ftNode.fromNode << endl
           << "     toNode " << sArc.ftNode.toNode << endl

           << sArc.arcGeom->toString() << endl;

        cout << ss.str() << endl;*/

        string extNodeID = "1";
        double supply = 5.0;
        NewNode startNode {extNodeID, coord, supply};
        NewNode endNode {extNodeID, geos::geom::Coordinate {703342, 5364710}, supply};

        unsigned int newEndNodeID = net.AddEndNode(startNode, cnfg.Treshold, *qry, false);
        cout << "New End Node ID: " << newEndNodeID << endl;

        newEndNodeID = net.AddEndNode(endNode, cnfg.Treshold, *qry, false);
        cout << "New End Node ID: " << newEndNodeID << endl;

        unsigned int newStartNodeID = net.AddStartNode(startNode, cnfg.Treshold, *qry, false);
        cout << "New Start Node ID: " << newStartNodeID << endl;

        newStartNodeID = net.AddStartNode(endNode, cnfg.Treshold, *qry, false);
        cout << "New Start Node ID: " << newStartNodeID << endl;

        cout << "Reset of Network.." << endl;
        net.Reset();

        newEndNodeID = net.AddEndNode(startNode, cnfg.Treshold, *qry, false);
        cout << "New End Node ID: " << newEndNodeID << endl;

        newEndNodeID = net.AddEndNode(endNode, cnfg.Treshold, *qry, false);
        cout << "New End Node ID: " << newEndNodeID << endl;

        newStartNodeID = net.AddStartNode(startNode, cnfg.Treshold, *qry, false);
        cout << "New Start Node ID: " << newStartNodeID << endl;

        newStartNodeID = net.AddStartNode(endNode, cnfg.Treshold, *qry, false);
        cout << "New Start Node ID: " << newStartNodeID << endl;
        //delete qry;

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();
    }
    catch (exception& ex)
    {
        LOGGER::LogError("AddStartNode: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}

void netxpert::Test::TestMST(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "c") + "}";
	cout << s << endl;
	netxpert::simple::MinimumSpanningTree simpleSolver(s);
	simpleSolver.Solve();
}

void netxpert::Test::TestSPT(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "c") + "}";
	cout << s << endl;
	netxpert::simple::ShortestPathTree simpleSolver(s);
	simpleSolver.Solve();
}

void netxpert::Test::TestODMatrix(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "c") + "}";
	cout << s << endl;
	netxpert::simple::OriginDestinationMatrix simpleSolver(s);
	simpleSolver.Solve(true);
}

void netxpert::Test::TestTransportation(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "c") + "}";
	cout << s << endl;
	netxpert::simple::Transportation simpleSolver(s);
	simpleSolver.Solve();
}

void netxpert::Test::TestMCF(Config& cnfg)
{
	string s = UTILS::SerializeObjectToJSON<Config>(cnfg, "c") + "}";
	cout << s << endl;
	netxpert::simple::MinCostFlow simpleSolver(s);
	simpleSolver.Solve();
}

void netxpert::Test::TestTransportationExt(Config& cnfg)
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

        string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString(); //@"C:\data\TRANSPRT_40.sqlite";
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"***REMOVED***_LINE_edges";

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;
        string resultTableName = cnfg.ArcsTableName + "_transpext";
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

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();

        //input arcs to extODMatrix
        std::vector<ExtSPTreeArc> extODMatrix;
        for (InputArc& a : arcsTable)
		{
			extODMatrix.push_back(ExtSPTreeArc{ a.extArcID, ExternalArc{ a.extFromNode, a.extToNode }, a.cost });
        }
        //input nodes to extNodeSupply - no coords!
        vector<ExtNodeSupply> extNodeSupply;
        for (NewNode& node : nodesTable)
        {
            extNodeSupply.push_back( ExtNodeSupply {node.extNodeID, node.supply} );
        }
        //Transportation Solver
        Transportation transp(cnfg);
        //TransportationExt -> external setting of ODMatrix and nodesupply

        ExtTransportationData input {extODMatrix, extNodeSupply};

        transp.SetExtODMatrix(input.odm);
        transp.SetExtNodeSupply(input.supply);

        transp.Solve();

        Network net = *transp.net;

        LOGGER::LogInfo("Done!");
        LOGGER::LogInfo("Optimum: " + to_string(transp.GetOptimum()) );
        LOGGER::LogInfo("Count of Distributions: " + to_string(transp.GetDistribution().size()) );

        unordered_map<ODPair, DistributionArc> result = transp.GetDistribution();

        unique_ptr<DBWriter> writer;
        switch (cnfg.ResultDBType)
        {
            case RESULT_DB_TYPE::SpatiaLiteDB:
            {
                writer = unique_ptr<DBWriter> (new SpatiaLiteWriter(cnfg)) ;
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
            }
                break;
        }
        writer->CreateNetXpertDB();
        writer->OpenNewTransaction();
        writer->CreateSolverResultTable(resultTableName, NetXpertSolver::TransportationSolver, true);
        writer->CommitCurrentTransaction();
        LOGGER::LogDebug("Writing Geometries..");
        writer->OpenNewTransaction();

        vector<ExtDistributionArc> distArcs;

        unsigned int counter = 0;
        for (auto& dist : result)
        {
            counter += 1;
            if (counter % 2500 == 0)
                LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

            ODPair key = dist.first;
            DistributionArc val = dist.second;
            CompressedPath path = val.path;
            vector<unsigned int> ends = val.path.first;
            double costPerPath = path.second;
            double flow = val.flow;
            //TODO: get capacity per arc
            double cap = -1;

            cout << key.origin << "_>" << key.dest << endl;
            // only one arc
            unordered_set<string> arcIDs = net.GetOriginalArcIDs(vector<InternalArc>
                                                                        { InternalArc  {key.origin, key.dest} },
                                                                        transp.IsDirected);
            ExtArcID arcID = *arcIDs.begin();

            //auto route = transp.UncompressRoute(key.origin, ends);
            //cout << route.size() << endl;
            //cout << "From: " << key.origin << " To: " << key.dest << " flows: " << flow << " ,cost: " << costPerPath << endl;

            string orig;
            string dest;
            try{
                orig = net.GetOriginalNodeID(key.origin);
            }
            catch (exception& ex) {
                LOGGER::LogError(ex.what());
            }
            try{
                dest = net.GetOriginalNodeID(key.dest);
            }
            catch (exception& ex) {
                LOGGER::LogError(ex.what());
            }

            //JSON output!
            distArcs.push_back( ExtDistributionArc {arcID, ExternalArc {orig, dest}, costPerPath, flow });

            /*net.BuildTotalRouteGeometry(orig, dest, costPerPath, -1, flow, "", route,
                                        resultTableName, *writer);*/

        }
        writer->CommitCurrentTransaction();
        writer->CloseConnection();

        TransportationResult transpRes {transp.GetOptimum(), distArcs};
        cout << UTILS::SerializeObjectToJSON<TransportationResult>(transpRes, "result") + "\n }"<< endl;
        LOGGER::LogDebug("Done!");

    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestTransportationExt: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}


/*void netxpert::Test::TestMCF(Config& cnfg)
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

        string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString(); //@"C:\data\TRANSPRT_40.sqlite";
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"***REMOVED***_LINE_edges";

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;
        string resultTableName = cnfg.ArcsTableName + "_mcf";
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

        //MCF Solver
        MinCostFlow mcf(cnfg);
        mcf.Solve(net);
        LOGGER::LogInfo("Done!");
        LOGGER::LogInfo("Optimum: " + to_string(mcf.GetOptimum()) );
        LOGGER::LogInfo("Count of MCF: " + to_string(mcf.GetMinCostFlow().size()) );

        vector<FlowCost> result = mcf.GetMinCostFlow();

        unique_ptr<DBWriter> writer;
        switch (cnfg.ResultDBType)
        {
            case RESULT_DB_TYPE::SpatiaLiteDB:
            {
                writer = unique_ptr<DBWriter> (new SpatiaLiteWriter(cnfg)) ;
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
            }
                break;
        }
        writer->CreateNetXpertDB();
        writer->OpenNewTransaction();
        writer->CreateSolverResultTable(resultTableName, true);
        writer->CommitCurrentTransaction();
        LOGGER::LogDebug("Writing Geometries..");
        writer->OpenNewTransaction();
        int counter = 0;
        for (FlowCost& arcFlow : result)
        {
            counter += 1;
            if (counter % 2500 == 0)
                LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

            string arcIDs = "";
            InternalArc key = arcFlow.intArc;
            double cost = arcFlow.cost;
            double flow = arcFlow.flow;
            //TODO: get capacity per arc
            double cap = -1;
            vector<InternalArc> arc { key };

            vector<ArcData> arcData = net.GetOriginalArcData(arc, cnfg.IsDirected);
            // is only one arc
            if (arcData.size() > 0)
            {
                ArcData arcD = *arcData.begin();
                arcIDs = arcD.extArcID;
                cap = arcD.capacity;
            }

            string orig;
            string dest;
            try{
                orig = net.GetOriginalStartOrEndNodeID(key.fromNode);
            }
            catch (exception& ex) {
                orig = net.GetOriginalNodeID(key.fromNode);
            }
            try{
                dest = net.GetOriginalStartOrEndNodeID(key.toNode);
            }
            catch (exception& ex) {
                dest = net.GetOriginalNodeID(key.fromNode);
            }
            net.ProcessResultArcs(orig, dest, cost, cap, flow, arcIDs, arc, resultTableName, *writer);
        }
        writer->CommitCurrentTransaction();
        writer->CloseConnection();
        LOGGER::LogDebug("Done!");

    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestMCF: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}*/

/*
void netxpert::Test::TestMST(Config& cnfg)
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
        string arcsGeomColumnName = cnfg.ArcsGeomColumnName; //"Geometry";

        string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString(); //@"C:\data\TRANSPRT_40.sqlite";
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"***REMOVED***_LINE_edges";

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;
        string resultTableName = cnfg.ArcsTableName + "_mst";
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
        LOGGER::LogInfo("Done!");
        Network net (arcsTable, cmap, cnfg);

        LOGGER::LogInfo("Converting Data into internal network..");
        net.ConvertInputNetwork(autoCleanNetwork);

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();
        LOGGER::LogInfo("Done!");

        //MST Solver
        MinimumSpanningTree mst(cnfg);
        mst.Solve(net);
        LOGGER::LogInfo("Done!");
        LOGGER::LogInfo("Optimum: " + to_string(mst.GetOptimum()) );
        LOGGER::LogInfo("Count of MST: " + to_string(mst.GetMinimumSpanningTree().size()) );

        unique_ptr<DBWriter> writer;
        switch (cnfg.ResultDBType)
        {
            case RESULT_DB_TYPE::SpatiaLiteDB:
            {
                writer = unique_ptr<DBWriter> (new SpatiaLiteWriter(cnfg)) ;
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
            }
                break;
        }
        writer->CreateNetXpertDB();

        string arcIDs;
        vector<string> arcIDlist = net.GetOriginalArcIDs(mst.GetMinimumSpanningTree(), cnfg.IsDirected);
        for (string& id : arcIDlist)
        {
            // 13-14 min on 840000 arcs
            //arcIDs = arcIDs + id + ","; //+= is c++ concat operator!
            arcIDs += id += ","; //optimized! 0.2 seconds on 840000 arcs
        }
        arcIDs.pop_back(); //trim last comma

        LOGGER::LogDebug("Writing Geometries..");
        net.ProcessResultArcs("", "", -1, -1, -1, arcIDs, resultTableName);
        LOGGER::LogDebug("Done!");
    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestMST: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}*/
/*
void netxpert::Test::TestSPT(Config& cnfg)
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

        string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString(); //@"C:\data\TRANSPRT_40.sqlite";
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"***REMOVED***_LINE_edges";

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;

        string resultTableName = cnfg.ArcsTableName + "_spt";
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
        vector<pair<unsigned int, string>> endNodes;
        if (!cnfg.SPTAllDests) {
            LOGGER::LogInfo("Loading End nodes..");
            endNodes = net.LoadEndNodes(nodesTable, cnfg.Treshold, arcsTableName,
                                                                        cnfg.ArcsGeomColumnName, cmap, withCapacity);
        }

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();

        // Solver
        ShortestPathTree spt(cnfg);

        spt.SetOrigin(startNodes.at(0).first);
        vector<unsigned int> dests = {};// newEndNodeID, newEndNodeID2}; //newEndNodeID}; // {}

        if (!cnfg.SPTAllDests)
        {
            for (auto d : endNodes)
                dests.push_back(d.first);
        }
        spt.SetDestinations( dests );

        spt.Solve(net);

        LOGGER::LogInfo("Optimum: " + to_string(spt.GetOptimum()) );
        LOGGER::LogInfo("Count of SPT: " +to_string( spt.GetShortestPaths().size() ) );

        auto kvSPS = spt.GetShortestPaths();

        unique_ptr<DBWriter> writer;
        switch (cnfg.ResultDBType)
        {
            case RESULT_DB_TYPE::SpatiaLiteDB:
            {
                writer = unique_ptr<DBWriter> (new SpatiaLiteWriter(cnfg)) ;
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
            }
                break;
        }
        writer->OpenNewTransaction();
        writer->CreateNetXpertDB();
        writer->CreateSolverResultTable(resultTableName, true);
        writer->CommitCurrentTransaction();
        LOGGER::LogDebug("Writing Geometries..");
        writer->OpenNewTransaction();
        int counter = 0;
        for (auto kv : kvSPS)
        {
            counter += 1;
            string arcIDs = "";
            ODPair key = kv.first;
            CompressedPath value = kv.second;
            vector<unsigned int> ends = value.first;
            double costPerPath = value.second;

            auto route = spt.UncompressRoute(key.origin, ends);

            vector<string> arcIDlist = net.GetOriginalArcIDs(route, cnfg.IsDirected);

            if (arcIDlist.size() > 0)
            {
                for (string& id : arcIDlist)
                    arcIDs += id += ",";
                arcIDs.pop_back(); //trim last comma
            }
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
            net.ProcessResultArcs(orig, dest, costPerPath, -1, -1, arcIDs, route, resultTableName, *writer);
        }
        writer->CommitCurrentTransaction();
        writer->CloseConnection();
        LOGGER::LogDebug("Done!");
    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestSPT: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}
*/

/*
void netxpert::Test::TestODMatrix(Config& cnfg)
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

        string pathToSpatiaLiteDB = cnfg.SQLiteDBPath; //args[0].ToString(); //@"C:\data\TRANSPRT_40.sqlite";
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"***REMOVED***_LINE_edges";

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;
        string resultTableName = cnfg.ArcsTableName + "_odm";
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

        DBHELPER::CommitCurrentTransaction();
        DBHELPER::CloseConnection();

        // Solver
        OriginDestinationMatrix odm(cnfg);
        vector<unsigned int> origs = {}; //newStartNodeID, newStartNodeID2};
        for (auto s : startNodes)
            origs.push_back(s.first);

        odm.SetOrigins( origs );

        vector<unsigned int> dests = {}; //newEndNodeID, newEndNodeID2}; //newEndNodeID}; // {}
        for (auto e : endNodes)
            dests.push_back(e.first);

        odm.SetDestinations( dests );

        odm.Solve(net);

        LOGGER::LogInfo("Optimum: " + to_string(odm.GetOptimum()) );
        LOGGER::LogInfo("Count of ODMatrix: " +to_string( odm.GetODMatrix().size() ) );
        //LOGGER::LogInfo("Count of SPS: " +to_string( odm.GetShortestPaths().size() ) );

        auto kvSPS = odm.GetShortestPaths();
        unique_ptr<DBWriter> writer;
        unique_ptr<SQLite::Statement> qry;
        switch (cnfg.ResultDBType)
        {
            case RESULT_DB_TYPE::SpatiaLiteDB:
            {
                writer = unique_ptr<DBWriter> (new SpatiaLiteWriter(cnfg)) ;
                 if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
                {
                    auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
                    qry = unique_ptr<SQLite::Statement> (sldbWriter.PrepareSaveResultArc(resultTableName));
                }
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
            }
                break;
        }
        writer->CreateNetXpertDB();
        writer->OpenNewTransaction();
        writer->CreateSolverResultTable(resultTableName, true);
        writer->CommitCurrentTransaction();
        LOGGER::LogDebug("Writing Geometries..");
        writer->OpenNewTransaction();
        int counter = 0;
        for (auto& kv : kvSPS)
        {
            counter += 1;
            if (counter % 2500 == 0)
                LOGGER::LogInfo("Processed #" + to_string(counter) + " geometries.");

            string arcIDs = "";
            ODPair key = kv.first;
            CompressedPath value = kv.second;
            vector<unsigned int> ends = value.first;
            double costPerPath = value.second;

            auto route = odm.UncompressRoute(key.origin, ends);

            vector<string> arcIDlist = net.GetOriginalArcIDs(route, cnfg.IsDirected);

            if (arcIDlist.size() > 0)
            {
                for (string& id : arcIDlist)
                    arcIDs += id += ",";
                arcIDs.pop_back(); //trim last comma
            }
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
            net.ProcessResultArcs(orig, dest, costPerPath, -1, -1, arcIDs, route, resultTableName, *writer, *qry);
        }
        writer->CommitCurrentTransaction();
        writer->CloseConnection();
        LOGGER::LogDebug("Done!");
    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestODMatrix: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}*/


/*
void netxpert::Test::TestTransportation(Config& cnfg)
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

        string pathToSpatiaLiteDB = cnfg.NetXDBPath; //args[0].ToString(); //@"C:\data\TRANSPRT_40.sqlite";
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"***REMOVED***_LINE_edges";

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;
        string resultTableName = cnfg.ArcsTableName + "_transp";
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
        Transportation transp(cnfg);

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
        unique_ptr<SQLite::Statement> qry;
        switch (cnfg.ResultDBType)
        {
            case RESULT_DB_TYPE::SpatiaLiteDB:
            {
                writer = unique_ptr<DBWriter> (new SpatiaLiteWriter(cnfg)) ;
                if (cnfg.GeometryHandling != GEOMETRY_HANDLING::RealGeometry)
                {
                    auto& sldbWriter = dynamic_cast<SpatiaLiteWriter&>(*writer);
                    qry = sldbWriter.PrepareSaveResultArc(resultTableName);
                }
            }
                break;
            case RESULT_DB_TYPE::ESRI_FileGDB:
            {
                writer = unique_ptr<DBWriter> (new FGDBWriter(cnfg)) ;
            }
                break;
        }
        writer->CreateNetXpertDB();
        writer->OpenNewTransaction();
        writer->CreateSolverResultTable(resultTableName, true);
        writer->CommitCurrentTransaction();
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

    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestTransportation: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}
*/
