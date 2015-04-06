#include "dbhelper.h"
#include <exception>

using namespace NetXpert;
using namespace geos::io;
using namespace geos::geom;

//Init static member variables must be out of class scope!
SQLite::Database* DBHELPER::connPtr = nullptr;
SQLite::Transaction* DBHELPER::currentTransactionPtr = nullptr;
Config DBHELPER::NETXPERT_CNFG;
bool DBHELPER::isConnected = false;
bool DBHELPER::IsInitialized = false;
unordered_set<string> DBHELPER::EliminatedArcs;
shared_ptr<GeometryFactory> DBHELPER::GEO_FACTORY;

namespace NetXpert {
    class GeometryEmptyException: public std::exception
    {
      virtual const char* what() const throw()
      {
        return "Geometry is empty!";
      }
    } GeometryEmptyException;
}

DBHELPER::~DBHELPER()
{
    //dtor
    if (connPtr)
        delete connPtr;
    if (currentTransactionPtr)
        delete currentTransactionPtr;
}

void DBHELPER::Initialize(const Config& cnfg)
{
    //FIXED = 0 Kommastellen
    //FLOATING_SINGLE = 6 Kommastellen
    //FLOATING = 16 Kommastellen
	shared_ptr<PrecisionModel> pm (new PrecisionModel( geos::geom::PrecisionModel::FLOATING_SINGLE));

	// Initialize global factory with defined PrecisionModel
	// and a SRID of -1 (undefined).
	DBHELPER::GEO_FACTORY = shared_ptr<GeometryFactory> ( new GeometryFactory( pm.get(), -1)); //SRID = -1

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
InputArcs DBHELPER::LoadNetworkFromDB(string _tableName,const ColumnMap& _map)
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
        return arcTbl;
    }
}
InputNodes DBHELPER::LoadNodesFromDB(string _tableName, const ColumnMap& _map)
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

std::unique_ptr<SQLite::Statement> DBHELPER::PrepareGetClosestArcQuery(string tableName, string geomColName,
                                                        const ColumnMap& cmap, ArcIDColumnDataType arcIDColDataType,
                                                        bool withCapacity)
{
    string eliminatedArcIDs = "";
    string sqlStr = "";

    try
    {
        switch (arcIDColDataType)
        {
            case ArcIDColumnDataType::Std_String:
                for (const string& elem: DBHELPER::EliminatedArcs) {
                    eliminatedArcIDs += ",'", elem, "'";
                }
                break;
            default: //double or int
                for (const string& elem: DBHELPER::EliminatedArcs) {
                    eliminatedArcIDs += ",", elem;
                }
                break;
        }
        //trim comma on first
        if (eliminatedArcIDs.length() > 0)
            eliminatedArcIDs = eliminatedArcIDs.erase(0,1);

        LOGGER::LogDebug("Eliminated Arcs: "+ eliminatedArcIDs);

        if (withCapacity)
        {
            sqlStr = "SELECT "+ cmap.arcIDColName +", "+ cmap.fromColName+", "+ cmap.toColName+", "+cmap.costColName+
                    ", "+ cmap.capColName +", AsBinary(" + geomColName +
                    ") as a_geometry, AsBinary(ST_ClosestPoint("
                    +geomColName+", MakePoint(@XCoord, @YCoord))) as p_geometry"+
                    " FROM "+tableName + " WHERE "+cmap.arcIDColName +" NOT IN ("+eliminatedArcIDs+")"+
                    " AND ST_Distance("+geomColName+", MakePoint(@XCoord, @YCoord)) < @Treshold"+
                    " AND ROWID IN"+
                    " (SELECT ROWID FROM SpatialIndex WHERE f_table_name = '"+tableName+"'"+
                       " AND search_frame = BuildCircleMbr(@XCoord, @YCoord, @Treshold))"+
                    " ORDER BY ST_Distance("+geomColName+", MakePoint(@XCoord, @YCoord)) LIMIT 1";
        }
        else
        {
            sqlStr = "SELECT "+ cmap.arcIDColName +", "+ cmap.fromColName+", "+ cmap.toColName+", "+cmap.costColName+
                    ", AsBinary(" + geomColName +
                    ") as a_geometry, AsBinary(ST_ClosestPoint("
                    +geomColName+", MakePoint(@XCoord, @YCoord))) as p_geometry"+
                    " FROM "+tableName + " WHERE "+cmap.arcIDColName +" NOT IN ("+eliminatedArcIDs+")"+
                    " AND ST_Distance("+geomColName+", MakePoint(@XCoord, @YCoord)) < @Treshold"+
                    " AND ROWID IN"+
                    " (SELECT ROWID FROM SpatialIndex WHERE f_table_name = '"+tableName+"'"+
                       " AND search_frame = BuildCircleMbr(@XCoord, @YCoord, @Treshold))"+
                    " ORDER BY ST_Distance("+geomColName+", MakePoint(@XCoord, @YCoord)) LIMIT 1";
        }

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

ExtClosestArcAndPoint DBHELPER::GetClosestArcFromPoint(Coordinate coord, int treshold,
                                                    SQLite::Statement& qry, bool withCapacity)
{
    string closestArcID;
    shared_ptr<Geometry> pGeomPtr = nullptr;
    shared_ptr<Geometry> aGeomPtr = nullptr; //kein shared_ptr, weil es zur Reference als Member eines structs kopiert wird
    // nach dem return geht das Ding out of scope und räumt sich selbst auf
    string extFromNode;
    string extToNode;
    double cost;
    double capacity;

    /*cout << withCapacity << endl;
    cout << coord.toString() << endl;
    cout << treshold << endl;*/

    try
    {
        const double x = coord.x;
        const double y = coord.y;

        // Bind values to the parameters of the SQL query
        qry.bind("@XCoord", x);
        qry.bind("@YCoord", y);
        qry.bind("@Treshold", treshold);

        WKBReader wkbReader(*DBHELPER::GEO_FACTORY);
        std::stringstream is(ios_base::binary|ios_base::in|ios_base::out);

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

            /*cout << fromNode << endl
                 << toNode << endl
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
            if (!aGeoCol.isNull())
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

            return result;
        }

        throw GeometryEmptyException;
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error getting closest Arc!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}

std::unique_ptr<SQLite::Statement> DBHELPER::PrepareIsPointOnArcQuery(string tableName, string arcIDColumnName,
                                        string geomColumnName, ArcIDColumnDataType arcIDColDataType )
{
    string eliminatedArcIDs = "";
    try
    {
        switch (arcIDColDataType)
        {
            case ArcIDColumnDataType::Std_String:
                for (const string& elem: DBHELPER::EliminatedArcs) {
                    eliminatedArcIDs += ",'", elem, "'";
                }
                break;
            default: //double or int
                for (const string& elem: DBHELPER::EliminatedArcs) {
                    eliminatedArcIDs += ",", elem;
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

bool DBHELPER::IsPointOnArc(Coordinate coords, string arcID, SQLite::Statement& qry)
{
    bool isPointOnLine = false;
    const double tolerance = 0.0000001;

    try
    {
        const double x = coords.x;
        const double y = coords.y;

        // Bind values to the parameters of the SQL query
        qry.bind("@XCoord", x);
        qry.bind("@YCoord", y);
        qry.bind("@Tolerance", tolerance);
        qry.bind("@ArcID", arcID);

        while (qry.executeStep())
        {
            SQLite::Column col = qry.getColumn(0);
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
