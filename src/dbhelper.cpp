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

#include "dbhelper.h"
#include <stdexcept>

using namespace std;
using namespace geos::io;
using namespace geos::geom;
using namespace netxpert;
using namespace netxpert::cnfg;
using namespace netxpert::io;
using namespace netxpert::data;
using namespace netxpert::utils;

//Init static member variables must be out of class scope!
std::unique_ptr<SQLite::Database> DBHELPER::connPtr = nullptr;
std::unique_ptr<SQLite::Transaction> DBHELPER::currentTransactionPtr = nullptr;
netxpert::cnfg::Config DBHELPER::NETXPERT_CNFG;
bool DBHELPER::isConnected = false;
bool DBHELPER::IsInitialized = false;
std::unordered_set<netxpert::data::extarcid_t> DBHELPER::EliminatedArcs;
std::shared_ptr<geos::geom::GeometryFactory> DBHELPER::GEO_FACTORY;
std::unordered_map<netxpert::data::extarcid_t, std::shared_ptr<geos::geom::LineString>> DBHELPER::KV_Network;


namespace netxpert {
    namespace io {
    /* must reside in a cpp-File */
    class GeometryEmptyException: public std::exception
    {
      virtual const char* what() const throw()
      {
        return "Geometry is empty!";
      }
    } GeometryEmptyException;
}
}

//gets not called, because everything else is static
DBHELPER::~DBHELPER()
{

}

void DBHELPER::Initialize(const Config& cnfg)
{
    //FIXED = 0 Kommastellen
    //FLOATING_SINGLE = 6 Kommastellen
    //FLOATING = 16 Kommastellen
    // Sollte FLOATING sein - sonst gibts evtl geometriefehler (Lücken beim CreateRouteGeometries())
    // Grund ist, dass SpatiaLite eine hohe Präzision hat und diese beim splitten von Linien natürlich auch hoch sein
    // muss.
    // Performance ist zu vernachlässigen, weil ja nur geringe Mengen an Geometrien eingelesen und verarbeitet werden
    // (nur die Kanten, die aufgebrochen werden)
    // --> Zusammengefügt werden die Kanten (Route) ja in der DB bei Spatialite (FGDB?).

	std::shared_ptr<PrecisionModel> pm (new PrecisionModel( geos::geom::PrecisionModel::FLOATING));

	// Initialize global factory with defined PrecisionModel
	// and a SRID of -1 (undefined).
	DBHELPER::GEO_FACTORY = std::shared_ptr<geos::geom::GeometryFactory> ( new GeometryFactory( pm.get(), -1)); //SRID = -1

    DBHELPER::NETXPERT_CNFG = cnfg;
    IsInitialized = true;
}

/*void DBHELPER::connect( )
{
    try
    {
        connPtr = unique_ptr<SQLite::Database>(new SQLite::Database (NETXPERT_CNFG.NetXDBPath, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE));
        const int cache_size_kb = 512000;
        SQLite::Database& db = *connPtr;

        string sqlStr = "PRAGMA cache_size=" + to_string(cache_size_kb); //DEFAULT: 2000
        SQLite::Statement cmd1(db, sqlStr);
        cmd1.executeStep();

        sqlStr = "PRAGMA locking_mode=NORMAL"; //default NORMAL
        SQLite::Statement cmd2(db, sqlStr);
        cmd2.executeStep();

        sqlStr = "PRAGMA journal_mode=OFF"; //default: DELETE
        SQLite::Statement cmd3(db, sqlStr);
        cmd3.executeStep();

        sqlStr = "PRAGMA synchronous=NORMAL"; //default: DELETE
        SQLite::Statement cmd4(db, sqlStr);
        cmd4.executeStep();

        //connPtr = db;

        //Depointerize "on-the-fly" to SQLite::Database&
        if ( performInitialCommand() )
        {
            LOGGER::LogDebug("Successfully performed initial spatialite command.");
        }
        else
        {
            LOGGER::LogError("Error performing initial spatialite command!");
        }
        isConnected = true;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError("Error connecting to SpatiaLiteDB!");
        LOGGER::LogError( ex.what() );
    }
}*/

void DBHELPER::initSpatialMetaData()
{

    SQLite::Database& db = *connPtr;
    SQLite::Statement query(db, "SELECT InitSpatialMetadata('1');");

    try {
        query.executeStep();
    }
    catch (std::exception& e) {
        LOGGER::LogError("Error loading initial SpatiaLite metadata!");
        LOGGER::LogError( e.what() );
    }
}

void DBHELPER::optimizeSQLiteCon()
{
    //optimizations for sqlite
    string sqlStr = "";
    const int cache_size_kb = 512000;
    SQLite::Database& db = *connPtr;

    sqlStr = "PRAGMA cache_size=" + to_string(cache_size_kb); //DEFAULT: 2000
    SQLite::Statement cmd1(db, sqlStr);
    cmd1.executeStep();

    sqlStr = "PRAGMA locking_mode=NORMAL"; //default NORMAL
    SQLite::Statement cmd2(db, sqlStr);
    cmd2.executeStep();

    sqlStr = "PRAGMA journal_mode=OFF"; //default: DELETE
    SQLite::Statement cmd3(db, sqlStr);
    cmd3.executeStep();

    sqlStr = "PRAGMA synchronous=NORMAL"; //default: DELETE
    SQLite::Statement cmd4(db, sqlStr);
    cmd4.executeStep();
}
void DBHELPER::connect(bool inMemory )
{
    try
    {
        string sqlStr = "";
        if (NETXPERT_CNFG.LoadDBIntoMemory)
        {
            LOGGER::LogInfo("Loading database into memory..");
            // Create empty memory db
            DBHELPER::connPtr = unique_ptr<SQLite::Database>(new SQLite::Database (":memory:",
                                        SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE));

            SQLite::Database& db = *connPtr;

            //optimizeSQLiteCon();

            if ( performInitialCommand() )
                LOGGER::LogDebug("Successfully performed initial spatialite command.");
            else
                LOGGER::LogError("Error performing initial spatialite command!");

            initSpatialMetaData();

            // Attach all from disk db and copy nodes and arcs into memory db
            sqlStr = "ATTACH DATABASE '" + NETXPERT_CNFG.NetXDBPath +"' AS netx; ";
            SQLite::Statement cmd(db, sqlStr);
            cmd.executeStep();

            sqlStr = "CREATE TABLE "+NETXPERT_CNFG.ArcsTableName +" AS "+
                    "SELECT * FROM netx."+ NETXPERT_CNFG.ArcsTableName;
            SQLite::Statement cmd2(db, sqlStr);
            cmd2.executeStep();

            sqlStr = "CREATE TABLE "+NETXPERT_CNFG.NodesTableName +" AS "+
                    "SELECT * FROM netx."+ NETXPERT_CNFG.NodesTableName +"; ";
            SQLite::Statement cmd3(db, sqlStr);
            cmd3.executeStep();

            sqlStr = "DETACH DATABASE netx;";
            SQLite::Statement cmd4(db, sqlStr);
            cmd4.executeStep();

            sqlStr = "BEGIN;";
            SQLite::Statement cmd5(db, sqlStr);
            cmd5.executeStep();

            sqlStr = "SELECT RecoverGeometryColumn('"+ NETXPERT_CNFG.NodesTableName +"','"
                        +NETXPERT_CNFG.NodesGeomColumnName +"',25832,'POINT',2);";
            SQLite::Statement cmd6(db, sqlStr);
            cmd6.executeStep();

            //LOGGER::LogInfo("RecoverGeometryColumn 1");

            sqlStr = "SELECT CreateSpatialIndex('"+ NETXPERT_CNFG.NodesTableName +"','"
                    +NETXPERT_CNFG.NodesGeomColumnName +"'); ";
            SQLite::Statement cmd7(db, sqlStr);
            cmd7.executeStep();

            //LOGGER::LogInfo("CreateSpatialIndex 1");

            sqlStr = "SELECT RecoverGeometryColumn('"+ NETXPERT_CNFG.ArcsTableName +"','"
                    +NETXPERT_CNFG.ArcsGeomColumnName +"',0,'LINESTRING',2); ";
            SQLite::Statement cmd8(db, sqlStr);
            cmd8.executeStep();

            //LOGGER::LogInfo("RecoverGeometryColumn 2");

            sqlStr = "SELECT CreateSpatialIndex('"+ NETXPERT_CNFG.ArcsTableName +"','"
                            +NETXPERT_CNFG.ArcsGeomColumnName +"'); ";
            SQLite::Statement cmd9(db, sqlStr);
            cmd9.executeStep();

            sqlStr = "COMMIT;";
            SQLite::Statement cmd10(db, sqlStr);
            cmd10.executeStep();

            //LOGGER::LogInfo("CreateSpatialIndex 2");
            isConnected = true;
            LOGGER::LogInfo("Done!");
        }
        else
        {
            LOGGER::LogInfo("No memory database..");
            DBHELPER::connPtr = unique_ptr<SQLite::Database>(new SQLite::Database (NETXPERT_CNFG.NetXDBPath, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE));
            optimizeSQLiteCon();

            if ( performInitialCommand() )
                LOGGER::LogDebug("Successfully performed initial spatialite command.");
            else
                LOGGER::LogError("Error performing initial spatialite command!");

            isConnected = true;
        }
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError("Error connecting to SpatiaLiteDB!");
        LOGGER::LogError( ex.what() );
    }
}
void DBHELPER::CommitCurrentTransaction()
{
    try
    {
        SQLite::Transaction& transaction = *currentTransactionPtr;
        transaction.commit();
        LOGGER::LogDebug("Successfully committed current transaction.");
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError("Error committing current transaction!");
        LOGGER::LogError( ex.what() );
    }
}
void DBHELPER::OpenNewTransaction()
{
    try
    {
        if (!isConnected)
            connect(NETXPERT_CNFG.LoadDBIntoMemory);

        SQLite::Database& db = *connPtr;
        currentTransactionPtr = unique_ptr<SQLite::Transaction>(new SQLite::Transaction (db));
        LOGGER::LogDebug(db.getFilename());
        LOGGER::LogDebug("Successfully opened new transaction.");
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError("Error opening new transaction!");
        LOGGER::LogError( ex.what() );
    }
}

void
 DBHELPER::LoadGeometryToMem(const std::string& _tableName, const ColumnMap& _map,
                                 const std::string& geomColumnName, const std::string& arcIDs)
{
    using namespace netxpert::data;

    string eliminatedArcIDs = "";
    string sqlStr = "";

    try
    {
        if (!isConnected)
            connect(NETXPERT_CNFG.LoadDBIntoMemory);

        for (const extarcid_t& elem: DBHELPER::EliminatedArcs) {
//            eliminatedArcIDs += ","+ std::to_string(elem);
            eliminatedArcIDs += ","+ elem;
        }

        //trim comma on first
        if (eliminatedArcIDs.length() > 0)
        {
            eliminatedArcIDs = eliminatedArcIDs.erase(0,1);
            sqlStr = "SELECT "+_map.arcIDColName +", AsBinary(CastToLineString(" + geomColumnName + "))"+
                 " FROM "+_tableName+" WHERE "+_map.arcIDColName+" NOT IN ("+ eliminatedArcIDs+") AND "+
                        _map.arcIDColName+ " IN("+ arcIDs +")";
        }
        else
        {
            sqlStr = "SELECT "+_map.arcIDColName +", AsBinary(CastToLineString(" + geomColumnName + "))"+
                 " FROM "+_tableName+" WHERE "+_map.arcIDColName+" IN("+ arcIDs +")";
        }
        //cout << sqlStr << endl;

        SQLite::Database& db = *connPtr;
        SQLite::Statement qry (db, sqlStr);

        shared_ptr<MultiLineString> aGeomPtr;
        WKBReader wkbReader(*DBHELPER::GEO_FACTORY);
        stringstream is(ios_base::binary|ios_base::in|ios_base::out);

        while(qry.executeStep())
        {
            SQLite::Column idCol = qry.getColumn(0);
            SQLite::Column geoCol = qry.getColumn(1);
            extarcid_t id;
            shared_ptr<LineString> aGeomPtr;

            if (!idCol.isNull())
            {
                id  = idCol.getText();
            }
            if (!geoCol.isNull())
            {
                const void* pVoid = geoCol.getBlob();
                const int sizeOfwkb = geoCol.getBytes();

                const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

                for (int i = 0; i < sizeOfwkb; i++)
                    is << bytes[i];

                aGeomPtr = shared_ptr<LineString>( dynamic_cast<LineString*>( wkbReader.read(is) ) );
            }
            DBHELPER::KV_Network.insert( make_pair(id, move(aGeomPtr)) );
        }
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error retrieving geometries!" );
        LOGGER::LogError( ex.what() );
    }
}

InputArcs
 DBHELPER::LoadNetworkFromDB(const std::string& _tableName, const ColumnMap& _map)
{
    InputArcs arcTbl;
    string sqlStr = "";
    bool oneway = false;
    bool hasCapacity = false;
    try
    {
        if (!isConnected)
            connect(NETXPERT_CNFG.LoadDBIntoMemory);

        SQLite::Database& db = *connPtr;

        if (!_map.onewayColName.empty())
            oneway = true;
        if (!_map.capColName.empty())
            hasCapacity = true;

        if (oneway && hasCapacity)
        {
            sqlStr = "SELECT "+_map.arcIDColName+","+
                               _map.fromColName+","+
                               _map.toColName+","+
                               _map.costColName+","+
                               _map.capColName +","+
                               _map.onewayColName +
                                " FROM "+ _tableName +
                                " ORDER BY "+_map.fromColName+ ";";

        }
        if (!oneway && hasCapacity)
        {
            sqlStr = "SELECT "+_map.arcIDColName+","+
                               _map.fromColName+","+
                               _map.toColName+","+
                               _map.costColName+","+
                               _map.capColName +
                                " FROM "+ _tableName +
                                " ORDER BY "+_map.fromColName+ ";";
        }
        if (oneway && !hasCapacity)
        {
            sqlStr = "SELECT "+_map.arcIDColName+","+
                               _map.fromColName+","+
                               _map.toColName+","+
                               _map.costColName+","+
                               _map.onewayColName +
                                " FROM "+ _tableName +
                                " ORDER BY "+_map.fromColName+ ";";
        }
        if (!oneway && !hasCapacity)
        {
            sqlStr = "SELECT "+_map.arcIDColName+","+
                               _map.fromColName+","+
                               _map.toColName+","+
                               _map.costColName+
                                " FROM "+ _tableName +
                                " ORDER BY "+_map.fromColName+ ";";
        }

        netxpert::utils::LOGGER::LogDebug(sqlStr);

        SQLite::Statement query(db, sqlStr);
        //fetch data

        if (oneway && hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id     = query.getColumn(0);
                const string fNode   = query.getColumn(1);
                const string tNode   = query.getColumn(2);
                const double cost    = query.getColumn(3);
                const double cap     = query.getColumn(4);
                const string _oneway = query.getColumn(5);
                arcTbl.push_back(InputArc {id,fNode,tNode,
                                            cost,cap,_oneway});
            }
        }
        if (!oneway && hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id     = query.getColumn(0);
                const string fNode   = query.getColumn(1);
                const string tNode   = query.getColumn(2);
                const double cost    = query.getColumn(3);
                const double cap     = query.getColumn(4);
                arcTbl.push_back(InputArc {id,fNode,tNode,
                                            cost,cap,""});
            }
        }
        if (oneway && !hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id     = query.getColumn(0);
                const string fNode   = query.getColumn(1);
                const string tNode   = query.getColumn(2);
                const double cost    = query.getColumn(3);
                const string _oneway = query.getColumn(4);
                arcTbl.push_back(InputArc {id,fNode,tNode,
                                            cost,DOUBLE_INFINITY,_oneway});
            }
        }
        if (!oneway && !hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id     = query.getColumn(0);
                const string fNode   = query.getColumn(1);
                const string tNode   = query.getColumn(2);
                const double cost    = query.getColumn(3);
                arcTbl.push_back(InputArc {id,fNode,tNode,
                                            cost, DOUBLE_INFINITY, ""});
            }
        }

        LOGGER::LogDebug("Successfully fetched network table data.");
        return arcTbl;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error preparing query!" );
        LOGGER::LogError( ex.what() );
        LOGGER::LogError( sqlStr);
        return arcTbl;
    }
}

std::vector<NetworkBuilderInputArc>
DBHELPER::LoadNetworkToBuildFromDB(const std::string& _tableName, const ColumnMap& _map)
{
	NetworkBuilderInputArcs arcTbl;
    string sqlStr = "";
    bool oneway = false;
    bool hasCapacity = false;
    string geomColName = NETXPERT_CNFG.ArcsGeomColumnName;

    try
    {
        if (!isConnected)
            connect(NETXPERT_CNFG.LoadDBIntoMemory);

        SQLite::Database& db = *connPtr;

        if (!_map.onewayColName.empty())
            oneway = true;
        if (!_map.capColName.empty())
            hasCapacity = true;

        if (oneway && hasCapacity)
        {
            sqlStr = "SELECT "+_map.arcIDColName+","+
                               _map.costColName+","+
                               _map.capColName +","+
                               _map.onewayColName +","+
                               "AsBinary(" + geomColName + ")"+
                                   " FROM "+ _tableName + ";";
        }
        if (!oneway && hasCapacity)
        {
            sqlStr = "SELECT "+_map.arcIDColName+","+
                               _map.costColName+","+
                               _map.capColName +","+
                               "AsBinary(" + geomColName + ")"+
                                   " FROM "+ _tableName + ";";
        }
        if (oneway && !hasCapacity)
        {
            sqlStr = "SELECT "+_map.arcIDColName+","+
                               _map.costColName+","+
                               _map.onewayColName + ","+
                               "AsBinary(" + geomColName + ")"+
                                   " FROM "+ _tableName + ";";
        }
        if (!oneway && !hasCapacity)
        {
            sqlStr = "SELECT "+_map.arcIDColName+","+
                               _map.costColName+ "," +
                               "AsBinary(" + geomColName + ")"+
                                   " FROM "+ _tableName + ";";
        }

        WKBReader wkbReader(*DBHELPER::GEO_FACTORY);
        std::stringstream is(ios_base::binary|ios_base::in|ios_base::out);

        //cout << sqlStr << endl;
        SQLite::Statement query(db, sqlStr);

        if (oneway && hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id            = query.getColumn(0);
                const double cost           = query.getColumn(1);
                const double cap            = query.getColumn(2);
                const string _oneway        = query.getColumn(3);
                SQLite::Column geoCol       = query.getColumn(4);

                const void* pVoid     = geoCol.getBlob();
                const int sizeOfwkb   = geoCol.getBytes();

                const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

                for (int i = 0; i < sizeOfwkb; i++)
                    is << bytes[i];

                auto lGeomPtr = std::unique_ptr<Geometry>( wkbReader.read(is) );
				//move to shared_ptr ist anscheinend ok!
                arcTbl.push_back(NetworkBuilderInputArc {id,cost,cap,_oneway, move( lGeomPtr )});
            }
        }
        if (!oneway && hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id             = query.getColumn(0);
                const double cost            = query.getColumn(1);
                const double cap             = query.getColumn(2);
                const string _oneway         = "";
                SQLite::Column geoCol        = query.getColumn(3);

                const void* pVoid     = geoCol.getBlob();
                const int sizeOfwkb   = geoCol.getBytes();

                const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

                for (int i = 0; i < sizeOfwkb; i++)
                    is << bytes[i];

                auto lGeomPtr = std::unique_ptr<Geometry>( wkbReader.read(is) );
				//move to shared_ptr ist anscheinend ok!
                arcTbl.push_back(NetworkBuilderInputArc {id,cost,cap,_oneway, move( lGeomPtr )});
            }
        }
        if (oneway && !hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id            = query.getColumn(0);
                const double cost           = query.getColumn(1);
                const string _oneway        = query.getColumn(2);
                const double cap            = DOUBLE_INFINITY;
                SQLite::Column geoCol       = query.getColumn(3);

                const void* pVoid     = geoCol.getBlob();
                const int sizeOfwkb   = geoCol.getBytes();

                const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

                for (int i = 0; i < sizeOfwkb; i++)
                    is << bytes[i];

                auto lGeomPtr = std::unique_ptr<Geometry>( wkbReader.read(is) );
				//move to shared_ptr ist anscheinend ok!
                arcTbl.push_back(NetworkBuilderInputArc {id,cost,cap,_oneway, move( lGeomPtr )});
            }
        }
        if (!oneway && !hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id            = query.getColumn(0);
                const double cost           = query.getColumn(1);
                const string _oneway        = "";
                const double cap            = DOUBLE_INFINITY;
                SQLite::Column geoCol       = query.getColumn(2);

                const void* pVoid     = geoCol.getBlob();
                const int sizeOfwkb   = geoCol.getBytes();

                const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

                for (int i = 0; i < sizeOfwkb; i++)
                    is << bytes[i];

                auto lGeomPtr = std::unique_ptr<Geometry>( wkbReader.read(is) );
				//move to shared_ptr ist anscheinend ok!
                arcTbl.push_back(NetworkBuilderInputArc {id,cost,cap,_oneway, move( lGeomPtr )});
            }
        }

        LOGGER::LogDebug("Successfully fetched network table data.");
        return arcTbl;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error preparing query!" );
        LOGGER::LogError( ex.what() );
        LOGGER::LogError( sqlStr);
        return arcTbl;
    }
}

std::vector<NewNode> DBHELPER::LoadNodesFromDB(const std::string& _tableName, const std::string& geomColName,
                                               const ColumnMap& _map)
{
    vector<NewNode> nodesTbl;
    shared_ptr<Geometry> pGeomPtr = nullptr;

    string sqlStr = "";
    try
    {
        if (!isConnected)
            connect(NETXPERT_CNFG.LoadDBIntoMemory);

        SQLite::Database& db = *connPtr;

        sqlStr = "SELECT "+_map.nodeIDColName+","+
                            _map.supplyColName+ ","+
                            "AsBinary(" + geomColName + ")"+
                            " FROM "+ _tableName + ";";

        WKBReader wkbReader(*DBHELPER::GEO_FACTORY);
        std::stringstream is(ios_base::binary|ios_base::in|ios_base::out);

        //cout << sqlStr << endl;
        SQLite::Statement query(db, sqlStr);
        //fetch data
        while (query.executeStep())
        {
            const string  id      = query.getColumn(0);
            double supply         = query.getColumn(1);
            SQLite::Column geoCol = query.getColumn(2);
            if (!geoCol.isNull())
            {
                const void* pVoid = geoCol.getBlob();
                const int sizeOfwkb = geoCol.getBytes();

                const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

                for (int i = 0; i < sizeOfwkb; i++)
                    is << bytes[i];

                pGeomPtr = shared_ptr<Geometry>( wkbReader.read(is) );
                shared_ptr<Point> pPtr (dynamic_pointer_cast<Point>(pGeomPtr));

                const Coordinate* cPtr = pPtr->getCoordinate();
                const Coordinate coord = *cPtr;

                nodesTbl.push_back(NewNode {id, coord, supply});
            }

        }
        LOGGER::LogDebug("Successfully fetched nodes table data.");

        query.reset();

        return nodesTbl;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error preparing query!" );
        LOGGER::LogError( ex.what() );
        LOGGER::LogError( sqlStr);
        return nodesTbl;
    }
}

std::unique_ptr<SQLite::Statement> DBHELPER::PrepareGetClosestArcQuery(const std::string& tableName,
                                                        const std::string& geomColName,
                                                        const ColumnMap& cmap,
                                                        const ArcIDColumnDataType arcIDColDataType,
                                                        const bool withCapacity)
{
    using namespace netxpert::data;

    string eliminatedArcIDs = "";
    string sqlStr = "";

    try
    {
        if (!isConnected)
            connect(NETXPERT_CNFG.LoadDBIntoMemory);

        switch (arcIDColDataType)
        {
            case ArcIDColumnDataType::Std_String:
                for (const extarcid_t& elem : DBHELPER::EliminatedArcs) {
//                    eliminatedArcIDs += ",'" + std::to_string(elem) + "'";
                    eliminatedArcIDs += ",'" + elem + "'";
                }
                break;
            default: //double or int
                for (const extarcid_t& elem : DBHELPER::EliminatedArcs) {
//                    eliminatedArcIDs += ",'" + std::to_string(elem) + "'";
                    eliminatedArcIDs += ",'" + elem + "'";
                }
                break;
        }

        //DEBUG
//        std::cout << eliminatedArcIDs << std::endl;

        //trim comma on first
        if (eliminatedArcIDs.length() > 0)
            eliminatedArcIDs = eliminatedArcIDs.erase(0,1);

        // If Point lies exactly on the line a minimal shift of the coordinates is necessary
        // to yield the nearest arc
        // tolerance x,y: 0.000000001;
        if (withCapacity)
        {
            sqlStr = "SELECT "+ cmap.arcIDColName +", "+ cmap.fromColName+", "+ cmap.toColName+", "+cmap.costColName+
                    ", "+ cmap.capColName +", AsBinary(" + geomColName +
                    ") as a_geometry, AsBinary(ST_ClosestPoint("
                    +geomColName+", ST_Translate(MakePoint(@XCoord, @YCoord),@Tolerance,@Tolerance,0))) as p_geometry"+
                    " FROM "+tableName + " WHERE "+cmap.arcIDColName +" NOT IN ("+eliminatedArcIDs+")"+
                    " AND ST_Distance("+geomColName+", MakePoint(@XCoord, @YCoord)) < @Treshold"+
                    " AND ROWID IN"+
                    " (SELECT ROWID FROM SpatialIndex WHERE f_table_name = '"+tableName+"'"+
                       " AND f_geometry_column = '" + geomColName + "'" + //fix for multi column geometry with spatial index
                       " AND search_frame = BuildCircleMbr(@XCoord, @YCoord, @Treshold))"+
                    " ORDER BY ST_Distance("+geomColName+", MakePoint(@XCoord, @YCoord)) LIMIT 1";
        }
        else
        {
            sqlStr = "SELECT "+ cmap.arcIDColName +", "+ cmap.fromColName+", "+ cmap.toColName+", "+cmap.costColName+
                    ", AsBinary(" + geomColName +
                    ") as a_geometry, AsBinary(ST_ClosestPoint("
                    +geomColName+", ST_Translate(MakePoint(@XCoord, @YCoord),@Tolerance,@Tolerance,0))) as p_geometry"+
                    " FROM "+tableName + " WHERE "+cmap.arcIDColName +" NOT IN ("+eliminatedArcIDs+")"+
                    " AND ST_Distance("+geomColName+", MakePoint(@XCoord, @YCoord)) < @Treshold"+
                    " AND ROWID IN"+
                    " (SELECT ROWID FROM SpatialIndex WHERE f_table_name = '"+tableName+"'"+
                       " AND f_geometry_column = '" + geomColName + "'" + //fix for multi column geometry with spatial index
                       " AND search_frame = BuildCircleMbr(@XCoord, @YCoord, @Treshold))"+
                    " ORDER BY ST_Distance("+geomColName+", MakePoint(@XCoord, @YCoord)) LIMIT 1";
        }

        SQLite::Database& db = *connPtr;
        unique_ptr<SQLite::Statement> qryPtr (new SQLite::Statement(db, sqlStr));
        LOGGER::LogDebug(db.getFilename());
        LOGGER::LogDebug("Table exists: " + to_string( db.tableExists(NETXPERT_CNFG.ArcsTableName) ) );
        //std::shared_ptr<SQLite::Statement> qryPtr (new SQLite::Statement(db, sqlStr));
        LOGGER::LogDebug("Successfully prepared query.");
        return qryPtr;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error preparing query!" );
        LOGGER::LogError( ex.what() );
        return nullptr;
    }
}

ExtClosestArcAndPoint DBHELPER::GetClosestArcFromPoint(const geos::geom::Coordinate& coord,
                                                       const int treshold, SQLite::Statement& qry,
                                                       const bool withCapacity)
{
    string closestArcID;
    shared_ptr<Geometry> pGeomPtr = nullptr;
    shared_ptr<Geometry> aGeomPtr = nullptr;

    string extFromNode;
    string extToNode;
    double cost = 0;
    double capacity = DOUBLE_INFINITY;
    double tolerance = 0.000000001;

    try
    {
        if (!isConnected)
            connect(NETXPERT_CNFG.LoadDBIntoMemory);

        const double x = coord.x;
        const double y = coord.y;

        // Bind values to the parameters of the SQL query
        qry.bind("@XCoord", x);
        qry.bind("@YCoord", y);
        qry.bind("@Treshold", treshold);
        qry.bind("@Tolerance", tolerance);

        WKBReader wkbReader(*DBHELPER::GEO_FACTORY);
        std::stringstream is(ios_base::binary|ios_base::in|ios_base::out);

        #ifdef DEBUG
        string qryStr = qry.getQuery();
        string s1 = UTILS::ReplaceAll(qryStr, "@XCoord", to_string(x));
        s1 = UTILS::ReplaceAll(s1,"@YCoord", to_string(y));
        s1 = UTILS::ReplaceAll(s1,"@Treshold", to_string(treshold));

        LOGGER::LogDebug(s1);
        #endif

        while (qry.executeStep())
        {
            SQLite::Column arcIDcol = qry.getColumn(0);
            if (!arcIDcol.isNull())
            {
                if (arcIDcol.isInteger())
                    closestArcID = to_string(arcIDcol.getInt());
                if (arcIDcol.isFloat())
                    closestArcID = to_string(arcIDcol.getDouble());
                if (arcIDcol.isText())
                    closestArcID = arcIDcol.getText();
                //cout << closestArcID << endl;
            }

            if (!qry.getColumn(1).isNull())
                extFromNode = static_cast<string>( qry.getColumn(1).getText() );

            if (!qry.getColumn(2).isNull())
                extToNode = static_cast<string>( qry.getColumn(2).getText() );

            if (!qry.getColumn(3).isNull())
                cost = qry.getColumn(3).getDouble();

            /*cout << closestArcID << endl << extFromNode << endl
                 << extToNode << endl
                 << cost << endl;*/

            int indxCount = 4;
            if (withCapacity)
            {
                if (!qry.getColumn(4).isNull())
                    capacity =  qry.getColumn(4).getDouble();
                indxCount += 1; //+1 if capacitry for getColumn()
            }

            //Arc geom
            SQLite::Column aGeoCol = qry.getColumn(indxCount); //4 without cap, 5 with cap
            if (!aGeoCol.isNull()) //check for not null
            {
                const void* pVoid = aGeoCol.getBlob();
                const int sizeOfwkb = aGeoCol.getBytes();

                const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

                for (int i = 0; i < sizeOfwkb; i++)
                    is << bytes[i];

                 aGeomPtr = shared_ptr<Geometry>( wkbReader.read(is) );
            }
            //Closest Point geom
            SQLite::Column pGeoCol = qry.getColumn(indxCount+1);//5 without cap, 6 with cap
            if (!pGeoCol.isNull())
            {
                const void* pVoid = pGeoCol.getBlob();
                const int sizeOfwkb = pGeoCol.getBytes();

                const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

                for (int i = 0; i < sizeOfwkb; i++)
                    is << bytes[i];

                pGeomPtr = shared_ptr<Geometry>( wkbReader.read(is) );
            }

        }
        if (aGeomPtr && pGeomPtr)
        {
            std::shared_ptr<Point> pPtr (dynamic_pointer_cast<Point>(pGeomPtr));
            //std::shared_ptr<const LineString> aPtr (dynamic_pointer_cast<const LineString>(aGeomPtr));
            const Coordinate* cPtr = pPtr->getCoordinate();
            const Coordinate coord = *cPtr;

            //aGeomPtr darf nicht eine reference aus einem shared_ptr sein, weil das Ding sonst nach return gekillt wird!
            const ExtClosestArcAndPoint result = {closestArcID, extFromNode, extToNode, cost, capacity, coord, aGeomPtr};
            //cout << "DBHELPER: " <<aGeomPtr->toString() << endl;

            qry.reset();

            return result;
        }
        else
            throw GeometryEmptyException;
    }
    catch (exception& ex)
    {
        qry.reset(); // sonst gibts errors "library out of sequence"
        LOGGER::LogError( "Error getting closest Arc!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}

std::unique_ptr<geos::geom::MultiLineString> DBHELPER::GetArcGeometriesFromMem(const std::string& arcIDs)
{
//    LOGGER::LogDebug("GetArcGeometriesFromMem()");
    try
    {
        vector<Geometry*> geoms;
        vector<string> arcIDList = UTILS::Split(arcIDs, ',');

        for (auto s : arcIDList)
        {
//            LOGGER::LogDebug("Querying arcid #" + s +"..");

            if (DBHELPER::EliminatedArcs.count(s) == 0 ) //filter out eliminated arcs
            {
                shared_ptr<LineString> g = DBHELPER::KV_Network.at(s);
                /*Geometry* gPtr = g.get();
                LineString* lPtr = dynamic_cast<LineString*>(gPtr);
                cout << lPtr->toString() << endl;
                cout << gPtr->toString() << endl;*/
                geoms.push_back( g.get() );
            }
        }
        unique_ptr<MultiLineString> route (DBHELPER::GEO_FACTORY->createMultiLineString(geoms));

        return route;
    }
    catch (exception& ex)
    {
        LOGGER::LogError( "GetArcGeometriesFromMem() - Error getting arc geometries!" );
        LOGGER::LogError( ex.what() );

        cout << "IDs of KV_Network are: " << endl;
        for (auto& kv : DBHELPER::KV_Network) {
            cout << kv.first << endl;
        }

        return nullptr;
    }
}

std::unique_ptr<geos::geom::MultiLineString> DBHELPER::GetArcGeometriesFromDB(const std::string& tableName,
                                                             const std::string& arcIDColumnName,
                                                             const std::string& geomColumnName,
                                                             const ArcIDColumnDataType arcIDColDataType,
                                                             const std::string& arcIDs)
{
    using namespace netxpert::data;
    string eliminatedArcIDs = "";
    string sqlStr = "";

    try
    {
        if (!isConnected)
            connect(NETXPERT_CNFG.LoadDBIntoMemory);

        switch (arcIDColDataType)
        {
            case ArcIDColumnDataType::Std_String:
                for (const extarcid_t& elem: DBHELPER::EliminatedArcs) {
//                    eliminatedArcIDs += ",'" + std::to_string(elem) + "'";
                    eliminatedArcIDs += ",'" + elem + "'";
                }
                break;
            default: //double or int
                for (const extarcid_t& elem: DBHELPER::EliminatedArcs) {
//                    eliminatedArcIDs += ",'" + std::to_string(elem) + "'";
                    eliminatedArcIDs += "," + elem;
                }
                break;
        }
        //trim comma on first
        if (eliminatedArcIDs.length() > 0)
            eliminatedArcIDs = eliminatedArcIDs.erase(0,1);

        //ST_COLLECT müsste ok sein, weil der Multilinestring ja später sowieso per LineMerger mit den
        // anderen Teilen zusammengeführt wird. Schneller ist ST_Collect ggü. ST_union.
        //ODM_Big: 7:30 zu 8:30 min
        sqlStr = "SELECT AsBinary(CastToMultiLineString(ST_COLLECT(" + geomColumnName + ")))"+
                 " FROM "+tableName+" WHERE "+arcIDColumnName+" NOT IN ("+ eliminatedArcIDs+") AND "+
                        arcIDColumnName+ " IN("+ arcIDs +")";

        //LOGGER::LogDebug("Eliminated Arcs: "+ eliminatedArcIDs);

        //cout << sqlStr << endl;

        SQLite::Database& db = *connPtr;
        SQLite::Statement qry (db, sqlStr);

        unique_ptr<MultiLineString> aGeomPtr;
        WKBReader wkbReader(*DBHELPER::GEO_FACTORY);
        stringstream is(ios_base::binary|ios_base::in|ios_base::out);

        qry.executeStep(); //one row query -> no while necessary

        SQLite::Column col = qry.getColumn(0);

        if (!col.isNull())
        {
            const void* pVoid = col.getBlob();
            const int sizeOfwkb = col.getBytes();

            const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

            for (int i = 0; i < sizeOfwkb; i++)
                is << bytes[i];

            aGeomPtr = unique_ptr<MultiLineString>( dynamic_cast<MultiLineString*>( wkbReader.read(is) ) );
        }

        return aGeomPtr;

    }
    catch (exception& ex)
    {
        LOGGER::LogError( "GetArcGeometriesFromDB() - Error getting arc geometries!" );
        LOGGER::LogError( ex.what() );
        return nullptr;
    }
}

std::unique_ptr<geos::geom::MultiLineString> DBHELPER::GetArcGeometryFromDB(const std::string& tableName,
                                                             const std::string& arcIDColumnName,
                                                             const std::string& geomColumnName,
                                                             const ArcIDColumnDataType arcIDColDataType,
                                                             const netxpert::data::extarcid_t& arcID)
{
    using namespace netxpert::data;
    string eliminatedArcIDs = "";
    string sqlStr = "";

    try
    {
        if (!isConnected)
            connect(NETXPERT_CNFG.LoadDBIntoMemory);

        switch  (arcIDColDataType) {
            case ArcIDColumnDataType::Number: {
                sqlStr = "SELECT AsBinary(CastToMultiLineString(" + geomColumnName + "))"+
//                        " FROM "+tableName+" WHERE "+arcIDColumnName+ " = " + std::to_string(arcID) ;
                        " FROM "+tableName+" WHERE "+arcIDColumnName+ " = " + arcID ;

            }
            case ArcIDColumnDataType::Std_String: {
                sqlStr = "SELECT AsBinary(CastToMultiLineString(" + geomColumnName + "))"+
                        " FROM "+tableName+" WHERE "+arcIDColumnName+ " = '" + arcID +"'";
            }
        }

        //cout << sqlStr << endl;

        SQLite::Database& db = *connPtr;
        SQLite::Statement qry (db, sqlStr);

        unique_ptr<MultiLineString> aGeomPtr;
        WKBReader wkbReader(*DBHELPER::GEO_FACTORY);
        stringstream is(ios_base::binary|ios_base::in|ios_base::out);

        qry.executeStep(); //one row query -> no while necessary

        SQLite::Column col = qry.getColumn(0);

        if (!col.isNull())
        {
            const void* pVoid = col.getBlob();
            const int sizeOfwkb = col.getBytes();

            const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

            for (int i = 0; i < sizeOfwkb; i++)
                is << bytes[i];

            aGeomPtr = unique_ptr<MultiLineString>( dynamic_cast<MultiLineString*>( wkbReader.read(is) ) );
        }

        return aGeomPtr;

    }
    catch (exception& ex)
    {
        LOGGER::LogError( "Error getting arc geometry!" );
        LOGGER::LogError( ex.what() );
        return nullptr;
    }
}


std::unordered_set<netxpert::data::extarcid_t>
 DBHELPER::GetIntersectingArcs(const std::string& barrierTableName,
                                  const std::string& barrierGeomColName,
                                  const std::string& arcsTableName,
                                  const std::string& arcIDColName,
                                  const std::string& arcGeomColName)
{
    using namespace netxpert::data;

    string sqlStr = "";
    unordered_set<extarcid_t> arcIDs;

    try
    {
        if (!isConnected)
            connect(NETXPERT_CNFG.LoadDBIntoMemory);

        sqlStr = "SELECT " +arcIDColName+
                 " FROM "+arcsTableName+" a, "+barrierTableName + " b "+
                 " WHERE ST_Intersects(a."+arcGeomColName+",b."+barrierGeomColName+") = 1"+
                 " AND a.ROWID IN "+
                 " (SELECT ROWID FROM SpatialIndex WHERE f_table_name = '"+arcsTableName +"' "+
                 " AND f_geometry_column = '" + arcGeomColName + "'" + //fix for multi column geometry with spatial index
                 " AND search_frame = b."+barrierGeomColName+")";

        SQLite::Database& db = *connPtr;
        SQLite::Statement qry (db, sqlStr);

        while (qry.executeStep())
        {
            SQLite::Column col = qry.getColumn(0);

            if (!col.isNull())
            {
                //Int vs string
                if (col.isText())
                {
                    auto value = col.getText();

                    if (typeid(extarcid_t).hash_code() == typeid(uint32_t).hash_code() )
//                        arcIDs.insert(std::stoul(value));
                        arcIDs.insert(value);


//                    if (typeid(extarcid_t).hash_code() == typeid(std::string).hash_code() )
//                        arcIDs.insert(value);
                }
                if (col.isInteger())
                {
                    auto value = col.getInt();

                    if (typeid(extarcid_t).hash_code() == typeid(uint32_t).hash_code() )
//                        arcIDs.insert(value);
                        arcIDs.insert(std::to_string(value));

//                    if (typeid(extarcid_t).hash_code() == typeid(std::string).hash_code() )
//                        arcIDs.insert(std::to_string(value));
                }
            }
        }
        return arcIDs;
    }
    catch (exception& ex)
    {
        LOGGER::LogError( "Error getting intersecting arc geometries!" );
        LOGGER::LogError( ex.what() );
        return arcIDs;
    }
}

std::vector<std::unique_ptr<geos::geom::Geometry>>
 DBHELPER::GetBarrierGeometriesFromDB(const std::string& barrierTableName,
                             const std::string& barrierGeomColName)
{
    using namespace netxpert::data;
    string sqlStr = "";
    vector<unique_ptr<Geometry>> result;

    try
    {
        if (!isConnected)
            connect(NETXPERT_CNFG.LoadDBIntoMemory);

        sqlStr = "SELECT AsBinary(" + barrierGeomColName + ")"+
                        " FROM "+barrierTableName ;

        //cout << sqlStr << endl;

        SQLite::Database& db = *connPtr;
        SQLite::Statement qry (db, sqlStr);


        WKBReader wkbReader(*DBHELPER::GEO_FACTORY);
        stringstream is(ios_base::binary|ios_base::in|ios_base::out);

        while (qry.executeStep())
        {
            SQLite::Column col = qry.getColumn(0);

            if (!col.isNull())
            {
                const void* pVoid = col.getBlob();
                const int sizeOfwkb = col.getBytes();

                const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

                for (int i = 0; i < sizeOfwkb; i++)
                    is << bytes[i];

                unique_ptr<Geometry> aGeomPtr( wkbReader.read(is) );
                result.push_back(move(aGeomPtr));
            }
        }

        return result;

    }
    catch (exception& ex)
    {
        LOGGER::LogError( "Error getting barrier geometries!" );
        LOGGER::LogError( ex.what() );
        return result;
    }
}



std::unique_ptr<geos::geom::MultiPoint> DBHELPER::GetArcVertexGeometriesByBufferFromDB(const std::string& tableName,
                                                              const std::string& geomColumnName,
                                                              const ArcIDColumnDataType arcIDColDataType,
                                                              const std::string& arcIDColumnName,
                                                              const double bufferVal,
                                                              const geos::geom::Coordinate& p)
{
    using namespace netxpert::data;
    string eliminatedArcIDs = "";
    string sqlStr = "";

    try
    {
        if (!isConnected) {
            connect(NETXPERT_CNFG.LoadDBIntoMemory);
        }

        switch (arcIDColDataType)
        {
            case ArcIDColumnDataType::Std_String:
                for (const extarcid_t& elem: DBHELPER::EliminatedArcs) {
//                    eliminatedArcIDs += ",'" + std::to_string(elem) + "'";
                    eliminatedArcIDs += ",'" + elem + "'";
                }
                break;
            default: //double or int
                for (const extarcid_t& elem: DBHELPER::EliminatedArcs) {
//                    eliminatedArcIDs += ",'" + std::to_string(elem) + "'";
                    eliminatedArcIDs += "," + elem;
                }
                break;
        }
        //trim comma on first
        if (eliminatedArcIDs.length() > 0)
            eliminatedArcIDs = eliminatedArcIDs.erase(0,1);

        //ST_COLLECT müsste ok sein, weil der Multilinestring ja später sowieso per LineMerger mit den
        // anderen Teilen zusammengeführt wird. Schneller ist ST_Collect ggü. ST_union.
        //ODM_Big: 7:30 zu 8:30 min

        // all vertices of line geometries
//        sqlStr = "SELECT AsBinary(CastToMultiPoint(ST_Collect(ST_DissolvePoints(" + geomColumnName + "))))"+
//                 " FROM "+tableName+" WHERE "+arcIDColumnName+" NOT IN ("+ eliminatedArcIDs+") AND "+
//                      " ST_Intersects("+ geomColumnName +", ST_Buffer(MakePoint(@XCoord,@YCoord),@Buffer))";

        // only label points ~ mid points of line geometries
        sqlStr = "SELECT AsBinary(CastToMultiPoint(ST_Collect(ST_PointOnSurface(" + geomColumnName + "))))"+
                 " FROM "+tableName+" WHERE "+arcIDColumnName+" NOT IN ("+ eliminatedArcIDs+") AND "+
                      " ST_Intersects("+ geomColumnName +", ST_Buffer(MakePoint(@XCoord,@YCoord),@Buffer))";

        // start, end and label (=mid) points of line geometries
//        sqlStr = "SELECT AsBinary(CastToMultiPoint(ST_Collect(ST_Collect(ST_Collect(ST_StartPoint(" + geomColumnName + "), ST_PointOnSurface("+geomColumnName+")), ST_Endpoint("
//                                                                                          +geomColumnName+")))))"+
//                 " FROM "+tableName+" WHERE "+arcIDColumnName+" NOT IN ("+ eliminatedArcIDs+") AND "+
//                      " ST_Intersects("+ geomColumnName +", ST_Buffer(MakePoint(@XCoord,@YCoord),@Buffer))";

        cout << sqlStr << endl;
        //LOGGER::LogDebug("Eliminated Arcs: "+ eliminatedArcIDs);
//        sqlStr = "SELECT 1";

        cout << "preparing db.." << endl;
        SQLite::Database& db = *connPtr;

        cout << "preparing qry.." << endl;

        SQLite::Statement qry (db, sqlStr);

        qry.bind("@XCoord", p.x);
        qry.bind("@YCoord", p.y);
        qry.bind("@Buffer", bufferVal);

        cout << "qry prepared()" << endl;

        string sql = qry.getQuery();
        string s1 = UTILS::ReplaceAll(sql, "@XCoord", to_string(p.x));
        s1 = UTILS::ReplaceAll(s1,"@YCoord", to_string(p.y));
        s1 = UTILS::ReplaceAll(s1,"@Buffer", to_string(bufferVal));
        cout << s1 << endl;

        unique_ptr<MultiPoint> aGeomPtr;
        WKBReader wkbReader(*DBHELPER::GEO_FACTORY);
        stringstream is(ios_base::binary|ios_base::in|ios_base::out);

        cout << "pre excecuteStep()" << endl;
        qry.executeStep(); //one row query -> no while necessary

        cout << "excecuteStep()" << endl;
        SQLite::Column col = qry.getColumn(0);
        cout << "getColumn()" << endl;
        if (!col.isNull())
        {
            const void* pVoid = col.getBlob();
            const int sizeOfwkb = col.getBytes();

            const unsigned char* bytes = static_cast<const unsigned char*>(pVoid);

            for (int i = 0; i < sizeOfwkb; i++)
                is << bytes[i];

            aGeomPtr = unique_ptr<MultiPoint>( dynamic_cast<MultiPoint*>( wkbReader.read(is) ) );
            cout << "after wkbReader()" << endl;
        }

        return aGeomPtr;

    }
    catch (exception& ex)
    {
        LOGGER::LogError( "GetArcVertexGeometriesByBufferFromDB() - Error getting arc vertex geometries!" );
        LOGGER::LogError( ex.what() );
        return nullptr;
    }
}

std::unique_ptr<SQLite::Statement> DBHELPER::PrepareIsPointOnArcQuery(string tableName, string arcIDColumnName,
                                        string geomColumnName, ArcIDColumnDataType arcIDColDataType )
{
    using namespace netxpert::data;

    string eliminatedArcIDs = "";
    try
    {
        switch (arcIDColDataType)
        {
            case ArcIDColumnDataType::Std_String:
                for (const extarcid_t& elem: DBHELPER::EliminatedArcs) {
//                    eliminatedArcIDs += ","+ std::to_string(elem);
                    eliminatedArcIDs += ",'"+ elem+ "'";
                }
                break;
            default: //double or int
                for (const extarcid_t& elem: DBHELPER::EliminatedArcs) {
//                    eliminatedArcIDs += ","+ std::to_string(elem);
                    eliminatedArcIDs += ","+ elem;
                }
                break;
        }
        //trim comma on first
        if (eliminatedArcIDs.length() > 0)
            eliminatedArcIDs = eliminatedArcIDs.erase(0,1);

        LOGGER::LogDebug("Eliminated Arcs: "+ eliminatedArcIDs);

        // Spatially equal is a special case..
        // even if the text representation of two geometries can be the same (and the DB returns "equal" from the
        // textual comparison - it does not have to mean, that they're really spatially equal!
        // Thus SnapToGrid is needed..
        // Tolerance: 0.000000001
        const string sqlStr = "SELECT ST_Equals(ST_SnapToGrid(ST_ClosestPoint("+geomColumnName+
                              ",MakePoint(@XCoord, @YCoord)),@Tolerance),"+
                              " ST_SnapToGrid(MakePoint(@XCoord, @YCoord),@Tolerance))"+
                              " FROM "+tableName+" WHERE "+arcIDColumnName+" NOT IN ("+ eliminatedArcIDs+") AND "+
                                arcIDColumnName+" = '@ArcID'";
        LOGGER::LogDebug(sqlStr);

        SQLite::Database& db = *connPtr;
        std::unique_ptr<SQLite::Statement> qryPtr (new SQLite::Statement(db, sqlStr));
        LOGGER::LogDebug("Successfully prepared query.");
        return qryPtr;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error preparing query!" );
        LOGGER::LogError( ex.what() );
        return nullptr;
    }
}

bool DBHELPER::IsPointOnArc(Coordinate coords, string arcID, shared_ptr<SQLite::Statement> qry)
{
    bool isPointOnLine = false;
    const double tolerance = 0.0000001;

    try
    {
        const double x = coords.x;
        const double y = coords.y;

        // Bind values to the parameters of the SQL query
        qry->bind("@XCoord", x);
        qry->bind("@YCoord", y);
        qry->bind("@Tolerance", tolerance);
        qry->bind("@ArcID", arcID);

        while (qry->executeStep())
        {
            SQLite::Column col = qry->getColumn(0);
            isPointOnLine = static_cast<bool>(col.getInt());
        }
        return isPointOnLine;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error testing for point on line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}


void DBHELPER::CloseConnection()
{
    //DBHELPER::CommitCurrentTransaction();
    //init destrcutor on SQLiteCpp::Database for DB disconnect
    DBHELPER::connPtr = nullptr;
    //auto* tmp = connPtr.release();
    //delete tmp;
    DBHELPER::isConnected = false;
    /*try
    {
        DBHELPER::cleanupPtr();
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error closing connection to NetXpert SpatiaLite DB!" );
        LOGGER::LogError( ex.what() );
    }*/
}

bool DBHELPER::performInitialCommand()
{
    try
    {
        SQLite::Database& db = *connPtr;

        const string spatiaLiteHome = NETXPERT_CNFG.SpatiaLiteHome;
        const string spatiaLiteCoreName = NETXPERT_CNFG.SpatiaLiteCoreName;

        const string pathBefore = UTILS::GetCurrentDir();
        //chdir to spatiallitehome
        //cout << "spatiaLiteHome: " << spatiaLiteHome << endl;
        UTILS::SetCurrentDir(spatiaLiteHome);
        /* Old way:
        db.enableExtensions();
        const string strSQL = "SELECT load_extension(@spatiaLiteCoreName,@spatiaLiteEntryPoint);";
        SQLite::Statement query(db, strSQL);
        query.bind("@spatiaLiteCoreName", spatiaLiteCoreName);
        query.bind("@spatiaLiteEntryPoint", "sqlite3_modspatialite_init");
        query.executeStep();
        db.disableExtensions(); */
        //spatialite > 4.2.0 : mod_spatialite should be used - not spatialite.dll | libspatialite.so
        //new way
        #ifdef _WIN32
        //on some QGIS Versions entry point is called "spatialite_init_ex" vs "sqlite3_modspatialite_init"
        db.loadExtension(spatiaLiteCoreName.c_str(), "spatialite_init_ex");
        #else
        db.loadExtension(spatiaLiteCoreName.c_str(), NULL);
        #endif
        UTILS::SetCurrentDir(pathBefore);

        //cout <<  boost::filesystem::current_path() << endl;
        return true;
    }
    catch (std::exception& e) {
        LOGGER::LogError("Error performing initial SpatiaLite command!");
        LOGGER::LogError( e.what() );
        return false;
    }
}
