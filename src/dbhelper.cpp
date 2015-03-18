#include "dbhelper.h"

using namespace NetXpert;

//Init static member variables must be out of class scope!
SQLite::Database* DBHELPER::connPtr = nullptr;
SQLite::Transaction* DBHELPER::currentTransactionPtr = nullptr;
Config DBHELPER::NETXPERT_CNFG;
bool DBHELPER::isConnected = false;
bool DBHELPER::IsInitialized = false;

DBHELPER::~DBHELPER()
{
    //dtor
    if (connPtr)
        delete connPtr;
    if (currentTransactionPtr)
        delete currentTransactionPtr;
}

void DBHELPER::Initialize(Config& cnfg)
{
    DBHELPER::NETXPERT_CNFG = cnfg;
    IsInitialized = true;
}

void DBHELPER::connect( )
{
    try
    {
        connPtr = new SQLite::Database (NETXPERT_CNFG.SQLiteDBPath, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
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
        if ( performInitialCommand(*connPtr) )
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
            connect();

        SQLite::Database& db = *connPtr;
        currentTransactionPtr = new SQLite::Transaction (db);
        LOGGER::LogDebug("Successfully opened new transaction.");
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError("Error opening new transaction!");
        LOGGER::LogError( ex.what() );
    }
}
InputArcs DBHELPER::LoadNetworkFromDB(string _tableName, ColumnMap _map)
{
    InputArcs arcTbl;
    string sqlStr = "";
    bool oneway = false;
    bool hasCapacity = false;
    try
    {
        if (!isConnected)
            connect();

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
                                   " FROM "+ _tableName + ";";
        }
        if (!oneway && hasCapacity)
        {
            sqlStr = "SELECT "+_map.arcIDColName+","+
                               _map.fromColName+","+
                               _map.toColName+","+
                               _map.costColName+","+
                               _map.capColName +
                                   " FROM "+ _tableName + ";";
        }
        if (oneway && !hasCapacity)
        {
            sqlStr = "SELECT "+_map.arcIDColName+","+
                               _map.fromColName+","+
                               _map.toColName+","+
                               _map.costColName+","+
                               _map.onewayColName +
                                   " FROM "+ _tableName + ";";
        }
        if (!oneway && !hasCapacity)
        {
            sqlStr = "SELECT "+_map.arcIDColName+","+
                               _map.fromColName+","+
                               _map.toColName+","+
                               _map.costColName+
                                   " FROM "+ _tableName + ";";
        }

        //cout << sqlStr << endl;
        SQLite::Statement query(db, sqlStr);
        //fetch data

        if (oneway && hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id     = query.getColumn(0);
                int fNode            = query.getColumn(1);
                int tNode            = query.getColumn(2);
                double cost          = query.getColumn(3);
                double cap           = query.getColumn(4);
                const string _oneway = query.getColumn(5);
                arcTbl.push_back(InputArc {id,static_cast<unsigned int>(fNode),static_cast<unsigned int>(tNode),
                                            cost,cap,_oneway});
            }
        }
        if (!oneway && hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id    = query.getColumn(0);
                int fNode           = query.getColumn(1);
                int tNode           = query.getColumn(2);
                double cost         = query.getColumn(3);
                double cap          = query.getColumn(4);
                arcTbl.push_back(InputArc {id,static_cast<unsigned int>(fNode),static_cast<unsigned int>(tNode),
                                            cost,cap,""});
            }
        }
        if (oneway && !hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id     = query.getColumn(0);
                int fNode            = query.getColumn(1);
                int tNode            = query.getColumn(2);
                double cost          = query.getColumn(3);
                const string _oneway = query.getColumn(4);
                arcTbl.push_back(InputArc {id,static_cast<unsigned int>(fNode),static_cast<unsigned int>(tNode),
                                            cost,DOUBLE_INFINITY,_oneway});
            }
        }
        if (!oneway && !hasCapacity)
        {
            while (query.executeStep())
            {
                const string  id     = query.getColumn(0);
                int fNode            = query.getColumn(1);
                int tNode            = query.getColumn(2);
                double cost          = query.getColumn(3);
                arcTbl.push_back(InputArc {id,static_cast<unsigned int>(fNode),static_cast<unsigned int>(tNode),
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
        return arcTbl;
    }
}
InputNodes DBHELPER::LoadNodesFromDB(string _tableName, ColumnMap _map)
{
    InputNodes nodesTbl;
    string sqlStr = "";
    try
    {
        if (!isConnected)
            connect();

        SQLite::Database& db = *connPtr;
        sqlStr = "SELECT "+_map.nodeIDColName+","+
                            _map.supplyColName+
                            " FROM "+ _tableName + ";";

        //cout << sqlStr << endl;
        SQLite::Statement query(db, sqlStr);
        //fetch data
        while (query.executeStep())
        {
            const string  id     = query.getColumn(0);
            double supply        = query.getColumn(1);
            nodesTbl.push_back(InputNode {id,supply});
        }
        LOGGER::LogDebug("Successfully fetched nodes table data.");
        return nodesTbl;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error preparing query!" );
        LOGGER::LogError( ex.what() );
        return nodesTbl;
    }
}
void DBHELPER::CloseConnection()
{
    try
    {

    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error closing connection to NetXpert SpatiaLite DB!" );
        LOGGER::LogError( ex.what() );
    }
}

bool DBHELPER::performInitialCommand(SQLite::Database& db)
{
    try {
        //cout << NETXPERT_CNFG.SpatiaLiteHome << endl;
        //cout << NETXPERT_CNFG.SpatiaLiteCoreName << endl;

        const string spatiaLiteHome = NETXPERT_CNFG.SpatiaLiteHome;
        const string spatiaLiteCoreName = NETXPERT_CNFG.SpatiaLiteCoreName;

        const string pathBefore = boost::filesystem::current_path().string();
        //chdir to spatiallitehome
        //cout << "spatiaLiteHome: " << spatiaLiteHome << endl;
        boost::filesystem::current_path(spatiaLiteHome);
        db.enableExtensions();

        const string strSQL = "SELECT load_extension(@spatiaLiteCoreName);";
        SQLite::Statement query(db, strSQL);
        query.bind("@spatiaLiteCoreName", spatiaLiteCoreName);
        query.executeStep();

        db.disableExtensions();

        boost::filesystem::current_path(pathBefore);
        //cout <<  boost::filesystem::current_path() << endl;
        return true;
    }
    catch (std::exception& e) {
        LOGGER::LogError("Error performing initial SpatiaLite command!");
        LOGGER::LogError( e.what() );
        return false;
    }
}
