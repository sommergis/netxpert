#include "test.h"

using namespace std;
using namespace NetXpert;

void NetXpert::Test::NetworkConvert(Config& cnfg)
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

        string pathToSpatiaLiteDB = cnfg.SQLiteDBPath; //args[0].ToString(); //@"C:\data\TRANSPRT_40.sqlite";
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"TRANSPRT_GES_LINE_edges";

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;

        bool autoCleanNetwork = cnfg.CleanNetwork;

        ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

        //2. Load Network
        arcsTable = DBHELPER::LoadNetworkFromDB(arcsTableName, cmap);
        Network net (arcsTable, cmap, cnfg);
        LOGGER::LogInfo("Converting Data into internal network..");
        net.ConvertInputNetwork(autoCleanNetwork);
        LOGGER::LogInfo("Done!");

        LOGGER::LogInfo("Original node ID of 1: " + net.GetOriginalNodeID(1) );
    }
    catch (exception& ex)
    {
        LOGGER::LogError("NetworkConvert: Unerwarteter Fehler!");
        LOGGER::LogError(ex.what());
    }
}
void NetXpert::Test::TestFileGDBWriter(Config& cnfg)
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
        geos::geom::MultiLineString* mlPtr = dynamic_cast<geos::geom::MultiLineString*>(geomPtr);

        FGDBWriter fgdb(cnfg);
        fgdb.CreateNetXpertDB();

        fgdb.CreateSolverResultTable(resultTblName, dropFirst);
        fgdb.OpenNewTransaction();
        fgdb.SaveSolveQueryToDB(orig, dest, 1.0, 99999.0, 1.0, *mlPtr, resultTblName, false);
        fgdb.CommitCurrentTransaction();
        fgdb.CloseConnection();

        delete mlPtr;
    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestFileGDBWriter: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}

void NetXpert::Test::TestSpatiaLiteWriter(Config& cnfg)
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
        //geos::geom::MultiLineString* mlPtr = dynamic_cast<geos::geom::MultiLineString*>(geomPtr);

        SpatiaLiteWriter sldb( cnfg );
        sldb.CreateNetXpertDB();
        sldb.OpenNewTransaction();
        sldb.CreateSolverResultTable(resultTblName, dropFirst);
        auto queryPtr = sldb.PrepareSaveSolveQueryToDB(resultTblName);
        //depointerization "on the fly"
        sldb.SaveSolveQueryToDB(orig, dest, 1.0, 99999.0, 1.0, *geomPtr, resultTblName, false, *queryPtr);
        sldb.CommitCurrentTransaction();
        delete queryPtr;

        //delete mlPtr;
    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestSpatiaLiteWriter: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}
void NetXpert::Test::TestAddStartNode(Config& cnfg)
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

        string pathToSpatiaLiteDB = cnfg.SQLiteDBPath; //args[0].ToString(); //@"C:\data\TRANSPRT_40.sqlite";
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"TRANSPRT_GES_LINE_edges";

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;

        bool autoCleanNetwork = cnfg.CleanNetwork;

        ColumnMap cmap { cnfg.ArcIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
                        cnfg.CostColumnName, cnfg.CapColumnName, cnfg.OnewayColumnName,
                        cnfg.NodeIDColumnName, cnfg.NodeSupplyColumnName };

        //2. Load Network
        LOGGER::LogInfo("Loading Data from DB..!");
        arcsTable = DBHELPER::LoadNetworkFromDB(arcsTableName, cmap);
        //nodesTable = DBHELPER::LoadNodesFromDB(nodesTableName, cmap);
        LOGGER::LogInfo("Done!");
        Network net (arcsTable, cmap, cnfg);
        LOGGER::LogInfo("Converting Data into internal network..");
        net.ConvertInputNetwork(autoCleanNetwork);
        LOGGER::LogInfo("Done!");

        auto qry = DBHELPER::PrepareGetClosestArcQuery(arcsTableName, cnfg.ArcIDColumnName, cnfg.ArcsGeomColumnName,
                                            ArcIDColumnDataType::Number);
        geos::geom::Coordinate coord = {703444, 5364720};
        LOGGER::LogDebug(to_string(coord.x) + " " + to_string(coord.y) );

        auto result = DBHELPER::GetClosestArcFromPoint(coord, cnfg.Treshold, *qry);

        LOGGER::LogDebug("Closest Arc ID: " + result.extArcID);
        LOGGER::LogDebug("Closest Point X: " + to_string(result.closestPoint.x));
        LOGGER::LogDebug("Closest Point Y: " + to_string(result.closestPoint.y));

        geos::io::WKTReader reader;
        //string wkt1 = "LINESTRING(1 2, 3 4)";
        string wkt1 = "LINESTRING(3 4, 1 2)";
        string wkt2 = "LINESTRING(20 30, 30 40)";
        shared_ptr<Geometry> geomPtr1 (reader.read(wkt1));
        shared_ptr<Geometry> geomPtr2 (reader.read(wkt2));

        NewArc a1 = {geomPtr1, AddedNodeType::StartEdge, 0.0, 0.0 };
        NewArc a2 = {geomPtr2, AddedNodeType::StartEdge, 0.0, 0.0 };
        NewArcs nArcs;
        FTNode pair1 = {1,2};
        FTNode pair2 = {2,3};

        nArcs.insert(make_pair( pair2, a2));
        nArcs.insert(make_pair( pair1, a1));

        geos::geom::Coordinate c2 = {2.3, 1.2};

        auto res = net.GetSplittedClosestNewArcToPoint(c2, 2, false, nArcs);
        stringstream ss;
        ss << "Splitted Closest New Arc is: " << endl
           << "     fromNode " << res.fromNode << endl
           << "     toNode " << res.toNode << endl
           << "     cost " << res.cost << endl
           << "     capacity " << res.capacity << endl
           << res.arcGeom->toString() << endl;

        LOGGER::LogDebug(ss.str());
        //delete qry;
    }
    catch (exception& ex)
    {
        LOGGER::LogError("AddStartNode: Unexpected Error!");
        LOGGER::LogError(ex.what());
    }
}
