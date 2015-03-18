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
        string arcsTableName = cnfg.ArcsTableName; //args[1].ToString(); //"***REMOVED***_LINE_edges";

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;

        bool autoCleanNetwork = cnfg.CleanNetwork;

        ColumnMap cmap { cnfg.EdgeIDColumnName, cnfg.FromNodeColumnName, cnfg.ToNodeColumnName,
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
        LOGGER::LogError("TestFileGDBWriter: Unerwarteter Fehler!");
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
        geos::geom::MultiLineString* mlPtr = dynamic_cast<geos::geom::MultiLineString*>(geomPtr);

        SpatiaLiteWriter sldb( cnfg );
        sldb.CreateNetXpertDB();
        sldb.OpenNewTransaction();
        sldb.CreateSolverResultTable(resultTblName, dropFirst);
        auto queryPtr = sldb.PrepareSaveSolveQueryToDB(resultTblName);
        //depointerization "on the fly"
        sldb.SaveSolveQueryToDB(orig, dest, 1.0, 99999.0, 1.0, *geomPtr, resultTblName, false, *queryPtr);
        sldb.CommitCurrentTransaction();
        delete queryPtr;

        delete mlPtr;
    }
    catch (exception& ex)
    {
        LOGGER::LogError("TestSpatiaLiteWriter: Unerwarteter Fehler!");
        LOGGER::LogError(ex.what());
    }
}
