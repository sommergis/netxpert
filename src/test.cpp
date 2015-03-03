#include "test.h"

using namespace std;
using namespace NetXpert;

void NetXpert::Test::NetworkConvert(Config& cnfg)
{
    try
    {
        //1. Config
        DBHELPER::NETXPERT_CNFG = cnfg;

        try
        {
            if (!LOGGER::IsInitialized)
            {
                LOGGER::NetXpertConfig = cnfg;
                LOGGER::Initialize();
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
        bool isDirected = cnfg.IsDirected; //Convert.ToBoolean(args[2]); //true

        string nodesTableName = cnfg.NodesTableName;
        string nodesGeomColName = cnfg.NodesGeomColumnName;

        int treshold = cnfg.Treshold;
        bool autoCleanNetwork = cnfg.CleanNetwork;

        ColumnMap cmap { {"fromColName", cnfg.FromNodeColumnName}, {"toColName", cnfg.ToNodeColumnName},
            {"arcIDColName", cnfg.EdgeIDColumnName}, {"costColName", cnfg.CostColumnName},
            {"nodeIDColName", cnfg.NodeIDColumnName},
            {"supplyColName", cnfg.NodeSupplyColumnName}};

        if (!cnfg.CapColumnName.empty())
           cmap.insert(make_pair("capColName", cnfg.CapColumnName));
        if (!cnfg.OnewayColumnName.empty())
           cmap.insert(make_pair("onewayColName", cnfg.OnewayColumnName));

        //2. Load Network
        arcsTable = DBHELPER::LoadNetworkFromDB(arcsTableName, cmap);
        Network net (arcsTable, cmap, cnfg);
        LOGGER::LogInfo("Converting Data into internal network..");
        net.ConvertInputNetwork(autoCleanNetwork);
        LOGGER::LogInfo("Done!");
    }
    catch (exception& ex)
    {
        LOGGER::LogError("ConvertToNetwork: Unerwarteter Fehler!");
        LOGGER::LogError(ex.what());
    }
}
