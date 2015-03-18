#include "slitewriter.h"

#ifdef SQLITECPP_ENABLE_ASSERT_HANDLER
 namespace SQLite
 {
     /// definition of the assertion handler enabled when SQLITECPP_ENABLE_ASSERT_HANDLER is defined in the project (CMakeList.txt)
     void assertion_failed(const char* apFile, const long apLine, const char* apFunc, const char* apExpr, const char* apMsg)
     {
         // Print a message to the standard error output stream, and abort the program.
         std::cerr << apFile << ":" << apLine << ":" << " error: assertion failed (" << apExpr << ") in " << apFunc << "() with message \"" << apMsg << "\"\n";
         std::abort();
     }
 }
 #endif

using namespace NetXpert;
using namespace geos::io;
using namespace boost::filesystem;

SpatiaLiteWriter::SpatiaLiteWriter(Config& cnfg)
{
    //NETXPERT_CNFG = cnfg;
    if ( !LOGGER::IsInitialized )
    {
        LOGGER::Initialize(cnfg);
    }
    LOGGER::LogInfo("SpatiaLiteWriter initialized.");
    connPtr = nullptr;
    currentTransactionPtr = nullptr;
}

SpatiaLiteWriter::~SpatiaLiteWriter()
{
    //dtor
    if (connPtr)
        delete connPtr;
    if (currentTransactionPtr)
        delete currentTransactionPtr;
}


void SpatiaLiteWriter::connect( )
{
    try
    {
       connPtr = new SQLite::Database (NETXPERT_CNFG.ResultDBPath, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);

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

void SpatiaLiteWriter::OpenNewTransaction()
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

void SpatiaLiteWriter::CommitCurrentTransaction()
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

void SpatiaLiteWriter::CreateNetXpertDB()
{
    try
    {
        //check for existence
        if ( exists(NETXPERT_CNFG.ResultDBPath) )
        {
            //LOGGER::LogWarning("FileGDB "+ NETXPERT_CNFG.ResultDBPath + " already exists and will be overwritten!");
            LOGGER::LogWarning("SpatiaLite DB "+ NETXPERT_CNFG.ResultDBPath + " already exists!");
            //DeleteGeodatabase( newPath );
            return;
        }
        SQLite::Database db(NETXPERT_CNFG.ResultDBPath, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
        initSpatialMetaData(db);
        LOGGER::LogInfo("NetXpert SpatiaLite DB " + db.getFilename() +" created successfully.");
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError("Error creating NetXpert SpatiaLite for results!");
        LOGGER::LogError( ex.what() );
    }
}
void SpatiaLiteWriter::CreateSolverResultTable(string _tableName)
{
    CreateSolverResultTable(_tableName, false);
}

void SpatiaLiteWriter::CreateSolverResultTable(string _tableName, bool dropFirst)
{
    try
    {
        if (!isConnected)
            connect();

        SQLite::Database& db = *connPtr;

        if (db.tableExists(_tableName) && dropFirst)
        {
            LOGGER::LogWarning("Table "+ _tableName + " already exists!");
            dropTable(_tableName);
        }
        if (db.tableExists(_tableName) && !dropFirst)
        {
            LOGGER::LogError("Table "+ _tableName + " already exists!");
            return;
        }

        createTable(_tableName);

        recoverGeometryColumn(_tableName, "geometry", "MULTILINESTRING");
        LOGGER::LogDebug("NetXpert Result Table successfully created.");

    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error creating NetXpert Result Table "+ _tableName + "!" );
        LOGGER::LogError( ex.what() );
    }
}

bool SpatiaLiteWriter::performInitialCommand(SQLite::Database& db)
{
    try {
        const string spatiaLiteHome = NETXPERT_CNFG.SpatiaLiteHome;
        const string spatiaLiteCoreName = NETXPERT_CNFG.SpatiaLiteCoreName;

        const string pathBefore = boost::filesystem::current_path().string();
        //chdir to spatiallitehome
        boost::filesystem::current_path(spatiaLiteHome);
        //cout << boost::filesystem::current_path() << endl;
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
void SpatiaLiteWriter::initSpatialMetaData(SQLite::Database& db)
{
    if ( performInitialCommand(db) )
    {
        LOGGER::LogDebug("Successfully performed initial spatialite command.");
    }
    else
    {
        LOGGER::LogError("Error performing initial spatialite command!");
    }

    SQLite::Statement query(db, "SELECT InitSpatialMetadata('1');");

    try {
        query.executeStep();
    }
    catch (std::exception& e) {
        LOGGER::LogError("Error loading initial SpatiaLite metadata!");
        LOGGER::LogError( e.what() );
    }
}
void SpatiaLiteWriter::createTable ( string _tableName )
{
    const string strSQL = "CREATE TABLE "+_tableName +
                           " (PK_UID INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "fromCoord TEXT,"
                            "toCoord TEXT,"
                            "fromNode TEXT,"
                            "toNode TEXT,"
                            "cost DOUBLE,"
                            "capacity DOUBLE,"
                            "flow DOUBLE,"
                            "geometry MULTILINESTRING)";
    //cout << strSQL << endl;

    SQLite::Database& db = *connPtr;
    SQLite::Statement query(db, strSQL);

    query.exec();
}

void SpatiaLiteWriter::dropTable (string _tableName)
{
    const string strSQL = "DROP TABLE "+_tableName;
    SQLite::Database& db = *connPtr;
    SQLite::Statement query(db, strSQL);
    query.exec();
}

void SpatiaLiteWriter::recoverGeometryColumn(string _tableName, string _geomColName, string _geomType)
{
    //tableName, geomColName, srid, geomType, dimension
    const string strSQL = "SELECT RecoverGeometryColumn(@tableName,@geomColName,0,@geomType,2)";
    const string strSQL2 = "SELECT CreateSpatialIndex(@tableName,@geomColName)";
    try
    {
        SQLite::Database& db = *connPtr;
        SQLite::Statement query(db, strSQL);
        query.bind("@tableName", _tableName);
        query.bind("@geomColName", _geomColName);
        query.bind("@geomType", _geomType);

        query.executeStep();

        SQLite::Statement query2 (db, strSQL2);
        query2.bind("@tableName", _tableName);
        query2.bind("@geomColName", _geomColName);

        query2.executeStep();
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error recovering geometry table!" );
        LOGGER::LogError( ex.what() );
    }
}

SQLite::Statement* SpatiaLiteWriter::PrepareSaveSolveQueryToDB(string _tableName)
{
    try
    {
        const string sqlStr = "INSERT INTO "+_tableName +"(fromNode,toNode,cost,capacity,flow,geometry) VALUES " +
                                           "(@orig,@dest,@cost,@cap,@flow,GeomFromWKB(@geom))";
        SQLite::Database& db = *connPtr;
        SQLite::Statement* query = new SQLite::Statement(db, sqlStr);
        LOGGER::LogDebug("Successfully prepared query.");
        return query;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error preparing query!" );
        LOGGER::LogError( ex.what() );
        return nullptr;
    }
}

void SpatiaLiteWriter::SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    geos::geom::Geometry& route, string _tableName,
                                    bool truncateBeforeInsert, SQLite::Statement& query)
{
    try
    {
        // Bind values to the parameters of the SQL query
        query.bind("@orig", orig);
        query.bind("@dest", dest);
        query.bind("@cost", cost);
        query.bind("@cap", capacity);
        query.bind("@flow", flow);

        WKBWriter writer;
        //cout<<"WKBtest: machine byte order: "<<BYTE_ORDER<<endl;
        std::stringstream oss (ios::out|ios::binary);

        writer.write(route, oss);

        //Get length
        oss.seekp(0, ios::end);
        stringstream::pos_type offset = oss.tellp();
        //oss.seekp(0, ios::beg); //set to the start of stream

        //DON'T
        //http://blog.sensecodons.com/2013/04/dont-let-stdstringstreamstrcstr-happen.html
        //const char* blob = oss.str().c_str();

        //CORRECT
        string s = oss.str();
        const char* blob = s.c_str();

        query.bind("@geom", blob, static_cast<int>(offset));

        query.exec();

    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error saving results to NetXpert SpatiaLite DB!" );
        LOGGER::LogError( ex.what() );
    }

}

void SpatiaLiteWriter::CloseConnection()
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
