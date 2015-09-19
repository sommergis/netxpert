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

using namespace netxpert;
using namespace geos::io;

SpatiaLiteWriter::SpatiaLiteWriter(Config& cnfg)
{
    this->NETXPERT_CNFG = cnfg;
    if ( !LOGGER::IsInitialized )
    {
        LOGGER::Initialize(cnfg);
    }
    LOGGER::LogInfo("SpatiaLiteWriter initialized.");
    this->connPtr = nullptr;
    this->currentTransactionPtr = nullptr;
    this->isConnected = false;
    this->dbPath = this->NETXPERT_CNFG.ResultDBPath;
}

SpatiaLiteWriter::SpatiaLiteWriter(Config& cnfg, std::string dbPath)
{
    this->NETXPERT_CNFG = cnfg;
    if ( !LOGGER::IsInitialized )
    {
        LOGGER::Initialize(cnfg);
    }
    LOGGER::LogInfo("SpatiaLiteWriter initialized.");
    this->connPtr = nullptr;
    this->currentTransactionPtr = nullptr;
    this->isConnected = false;
    this->dbPath = dbPath;
}

SpatiaLiteWriter::~SpatiaLiteWriter()
{
    //dtor
    /*if (connPtr)
        delete connPtr;
    if (currentTransactionPtr)
        delete currentTransactionPtr;*/
}


void SpatiaLiteWriter::connect( )
{
    try
    {

        // Pointer verursacht possible mem leaks ~70,000 bytes
        connPtr = unique_ptr<SQLite::Database > (new SQLite::Database (this->dbPath,
                                                                        SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE));
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
    catch (SQLite::Exception& ex)
    {
        LOGGER::LogError("Error connecting to SpatiaLiteDB '" + this->dbPath +"'");
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}

void SpatiaLiteWriter::OpenNewTransaction()
{
    try
    {
        if (!isConnected)
            connect();

        SQLite::Database& db = *connPtr;
        currentTransactionPtr = unique_ptr<SQLite::Transaction> (new SQLite::Transaction (db));

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
        if ( UTILS::FileExists(this->dbPath) )
        {
            //LOGGER::LogWarning("FileGDB "+ NETXPERT_CNFG.ResultDBPath + " already exists and will be overwritten!");
            LOGGER::LogWarning("SpatiaLite DB "+ this->dbPath + " already exists!");
            //DeleteGeodatabase( newPath );
            return;
        }
        SQLite::Database db(this->dbPath, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);

        initSpatialMetaData();
        LOGGER::LogInfo("NetXpert SpatiaLite DB " + db.getFilename() +" created successfully.");
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError("Error creating NetXpert SpatiaLite for results!");
        LOGGER::LogError( ex.what() );
    }
}
void SpatiaLiteWriter::CreateSolverResultTable(const string& _tableName)
{
    CreateSolverResultTable(_tableName, false);
}

void SpatiaLiteWriter::CreateSolverResultTable(const string& _tableName, bool dropFirst)
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

bool SpatiaLiteWriter::performInitialCommand()
{
    try
    {
        SQLite::Database& db = *connPtr;
        const string spatiaLiteHome = NETXPERT_CNFG.SpatiaLiteHome;
        const string spatiaLiteCoreName = NETXPERT_CNFG.SpatiaLiteCoreName;

        const string pathBefore = UTILS::GetCurrentDir();

        UTILS::SetCurrentDir(spatiaLiteHome);

        db.enableExtensions();

        const string strSQL = "SELECT load_extension(@spatiaLiteCoreName);";
        SQLite::Statement query(db, strSQL);
        query.bind("@spatiaLiteCoreName", spatiaLiteCoreName);
        query.executeStep();

        db.disableExtensions();

        //boost::filesystem::current_path(pathBefore);
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
void SpatiaLiteWriter::initSpatialMetaData()
{
    if (!isConnected)
        connect();

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
    query.reset();
}

void SpatiaLiteWriter::dropTable (string _tableName)
{
    const string strSQL = "DROP TABLE "+_tableName;
    SQLite::Database& db = *connPtr;
    SQLite::Statement query(db, strSQL);
    query.exec();
    query.reset();
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

std::unique_ptr<SQLite::Statement> SpatiaLiteWriter::PrepareSaveResultArc(const std::string& _tableName)
{
    try
    {
        if (!isConnected)
            connect();

        const string sqlStr = "INSERT INTO "+_tableName +"(fromNode,toNode,cost,capacity,flow,geometry) VALUES " +
                                           "(@orig,@dest,@cost,@cap,@flow,GeomFromWKB(@geom))";
        SQLite::Database& db = *connPtr;
        auto query = unique_ptr<SQLite::Statement>(new SQLite::Statement(db, sqlStr));

        //LOGGER::LogDebug("Successfully prepared query.");
        return query;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error preparing query!" );
        LOGGER::LogError( ex.what() );
        return nullptr;
    }
}

void SpatiaLiteWriter::SaveResultArc(string orig, string dest, double cost, double capacity, double flow,
                                    const Geometry& route, string _tableName, SQLite::Statement& query)
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
        query.reset();
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error saving results to NetXpert SpatiaLite DB!" );
        LOGGER::LogError( ex.what() );
    }
}
/**
*   Case: Route parts and original arc ids form a result arc.
*/
void SpatiaLiteWriter::MergeAndSaveResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                        string geomColumnName, string arcIDColumnName, string arcTableName,
                                        const string& arcIDs, const MultiLineString& mLine, string resultTableName)
{
    try
    {
        if (!isConnected)
            connect();

        mergeAndSaveResultArcs(orig, dest, cost, capacity, flow, geomColumnName, arcIDColumnName, arcTableName, arcIDs,
                                            mLine, resultTableName);
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Error creating route geometry in database!");
        throw ex;
    }
}


void SpatiaLiteWriter::mergeAndSaveResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                     string geomColumnName, string arcIDColumnName, string arcTableName,
                                     const string& arcIDs, const MultiLineString& mLine, string resultTableName)
{
    string sqlStr;
    if (arcIDs.size() > 0)
    {
        //Outputs SRID is always 0
        sqlStr = "INSERT INTO "+ resultTableName+" (fromNode, toNode, cost, capacity, flow, geometry) " +
                "SELECT @orig, @dest, @cost, @capacity, @flow, SetSRID(CastToMultiLineString(ST_LineMerge ( ST_Union ( " +
                " GeomFromWKB(@mLine)," + //splitted segments as multilinestring
                " (SELECT ST_Collect(" + geomColumnName +") " +
                "  FROM "+arcTableName+" WHERE "+arcIDColumnName+" IN ("+ arcIDs +") ) ) "+ //middle part
                "  ) ),0)";
    }
    else
    {
        //Outputs SRID is always 0
        sqlStr = "INSERT INTO "+ resultTableName+" (fromNode, toNode, cost, capacity, flow, geometry) " +
                "SELECT @orig, @dest, @cost, @capacity, @flow, SetSRID(CastToMultiLineString(ST_LineMerge ( ST_Union ( " +
                " GeomFromWKB(@mLine)" + //splitted segments as multilinestring
                " ) ) ),0)";
    }
    /*string printSqlStr = sqlStr;

    replace(printSqlStr, "GeomFromWKB(","GeomFromText(");
    replace(printSqlStr, "@mLine", mLine.toString());
    replace(printSqlStr, "@orig", orig);
    replace(printSqlStr, "@dest", dest);
    replace(printSqlStr, "@cost", to_string(cost));
    replace(printSqlStr, "@capacity", to_string(capacity));
    replace(printSqlStr, "@flow", to_string(flow));
    cout << printSqlStr << endl;*/

    SQLite::Database& db = *connPtr;
    SQLite::Statement qry(db, sqlStr);

    qry.bind("@orig", orig);
    qry.bind("@dest", dest);
    qry.bind("@cost", cost);
    qry.bind("@capacity", capacity);
    qry.bind("@flow", flow);

    //geo binding
    WKBWriter wkbWriter;
    std::stringstream oss(ios_base::binary|ios_base::out);

    oss.seekp(0, ios::beg); //reset stream position
    wkbWriter.write(mLine, oss);
    //Get length
    oss.seekp(0, ios::end);
    stringstream::pos_type offset = oss.tellp();
    //DON'T
    //http://blog.sensecodons.com/2013/04/dont-let-stdstringstreamstrcstr-happen.html
    //const char* blob = oss.str().c_str();
    //CORRECT
    string s = oss.str();
    const char* blob = s.c_str();
    qry.bind("@mLine", blob, static_cast<int>(offset));

    qry.exec();
}

/**
*   Case: Original arc ids form a result arc.
*/
void SpatiaLiteWriter::MergeAndSaveResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                            string geomColumnName, string arcIDColumnName, string arcTableName,
                                            const string& arcIDs, string resultTableName)
{
    try
    {
        if (!isConnected)
            connect();

        mergeAndSaveResultArcs(orig, dest, cost, capacity, flow, geomColumnName, arcIDColumnName, arcTableName, arcIDs,
                                            resultTableName);
        return;
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Error creating route geometry in database!");
        throw ex;
    }
}
void SpatiaLiteWriter::mergeAndSaveResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                                string geomColumnName, string arcIDColumnName, string arcTableName,
                                                const string& arcIDs, string resultTableName)
{
    //Outputs SRID is always 0
    const string sqlStr = "INSERT INTO "+ resultTableName+"(fromNode, toNode, cost, capacity, flow, geometry) " +
                " SELECT @orig, @dest, @cost, @capacity, @flow, SetSRID(CastToMultiLineString(" + geomColumnName +"),0) " +
                "  FROM "+arcTableName+" WHERE "+arcIDColumnName+" IN ("+ arcIDs +") ";

    //cout << sqlStr << endl;
    //cout << sqlStr.replace("GeomFromWKB(","GeomFromText(").replace("@mLine", mLine.toString()) << endl;

    SQLite::Database& db = *connPtr;
    SQLite::Statement qry(db, sqlStr);

    qry.bind("@orig", orig);
    qry.bind("@dest", dest);
    qry.bind("@cost", cost);
    qry.bind("@capacity", capacity);
    qry.bind("@flow", flow);

    qry.exec();
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
