#include "fgdbwriter.h"

using namespace netxpert;
using namespace std;
using namespace boost::filesystem;
using namespace FileGDBAPI;


FGDBWriter::FGDBWriter(Config& netxpertConfig)
{
    NETXPERT_CNFG = netxpertConfig;
    if ( !LOGGER::IsInitialized )
    {
        LOGGER::Initialize(netxpertConfig);
    }
    LOGGER::LogInfo("FGDBWriter initialized.");
    geodatabasePtr = nullptr;
    currentTblPtr = nullptr;
}

FGDBWriter::~FGDBWriter()
{
    //dtor
    /*if (currentTblPtr)
        delete currentTblPtr;
    if (geodatabasePtr)
        delete geodatabasePtr;*/
    //CloseConnection();
}

/*// convert UTF-8 string to wstring
std::wstring utf8_to_wstring (const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
}

// convert wstring to UTF-8 string
std::string wstring_to_utf8 (const std::wstring& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.to_bytes(str);
}*/


/**
 *  Converts string to wstring
 */
int StringToWString(std::wstring &ws, const std::string &s)
{
    std::wstring wsTmp(s.begin(), s.end());

    ws = wsTmp;

    return 0;
}
/**
 *  Converts wstring to string
 */
int WStringToString (std::string &s, const std::wstring &ws)
{
    string sTmp(ws.begin(), ws.end());

    s = sTmp;

    return 0;
}

void FGDBWriter::connect()
{
    try
    {
        geodatabasePtr = unique_ptr<Geodatabase>(new Geodatabase());

        wstring newPath;
        StringToWString(newPath, NETXPERT_CNFG.ResultDBPath);

        cout << "FGDB Path: "<< NETXPERT_CNFG.ResultDBPath << endl;

        fgdbError hr;
        wstring errorText;

        if ((hr = OpenGeodatabase(newPath, *geodatabasePtr) ) == S_OK)
        {
            LOGGER::LogDebug("Connected to NetXpert FileGDB "+ NETXPERT_CNFG.ResultDBPath);
        }
        else {
            ErrorInfo::GetErrorDescription(hr, errorText);
            string newErrorText;
            WStringToString(newErrorText, errorText);
            LOGGER::LogError("Error connecting to FileGDB "+ NETXPERT_CNFG.ResultDBPath + "!");
            LOGGER::LogError(newErrorText + " - Code: " +to_string(hr));
        }
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError("Error connecting to FileGDB!");
        LOGGER::LogError( ex.what() );
    }
}
void FGDBWriter::OpenNewTransaction()
{
    // Begin load only mode. This shuts off the update of all indexes.
    if (currentTblPtr)
    {
        currentTblPtr->LoadOnlyMode(true);
        currentTblPtr->SetWriteLock();
    }
}
void FGDBWriter::CommitCurrentTransaction()
{
    // End load only mode. This updates all indexes.
    if (currentTblPtr)
    {
        currentTblPtr->LoadOnlyMode(false);
        currentTblPtr->FreeWriteLock();
    }
}
void FGDBWriter::CreateNetXpertDB()
{
    try
    {
        wstring newPath;
        cout << "FGDB Path: "<< NETXPERT_CNFG.ResultDBPath << endl;
        //ASCII string to UTF16 String
        StringToWString(newPath, NETXPERT_CNFG.ResultDBPath);
        cout << "FGDB Path: "<< newPath << endl;
        // TODO: muss nicht ueberschrieben werden, wenns existiert
        if ( exists(NETXPERT_CNFG.ResultDBPath) )
        {
            //LOGGER::LogWarning("FileGDB "+ NETXPERT_CNFG.ResultDBPath + " already exists and will be overwritten!");
            LOGGER::LogWarning("FileGDB "+ NETXPERT_CNFG.ResultDBPath + " already exists!");
            //DeleteGeodatabase( newPath );
            return;
        }

        fgdbError hr;
        wstring errorText;
        Geodatabase gdb; //lokal!

        //valgrind shows errors (conditional jumps/ uninitialized values
        if ((hr = CreateGeodatabase( newPath, gdb )) == S_OK)
        {
            LOGGER::LogInfo("FileGDB "+ NETXPERT_CNFG.ResultDBPath + " created.");
        }
        else {

            ErrorInfo::GetErrorDescription(hr, errorText);
            string newErrorText;
            WStringToString(newErrorText, errorText);
            LOGGER::LogError("Error creating FileGDB "+ NETXPERT_CNFG.ResultDBPath + "!");
            LOGGER::LogError(newErrorText + " - Code: " +to_string(hr));
        }

    }
    catch (std::exception& ex)
    {
        LOGGER::LogError("Error creating NetXpert FileGDB for results!");
        LOGGER::LogError( ex.what() );
    }
}
void FGDBWriter::CreateSolverResultTable(string _tableName)
{
    try {
        if (!geodatabasePtr)
            connect();

        createTable ( _tableName );
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error creating NetXpert Result Table"+ _tableName + "!" );
        LOGGER::LogError( ex.what() );
    }
}
void FGDBWriter::CreateSolverResultTable(string _tableName, bool dropFirst)
{
    try
    {
        if (!geodatabasePtr)
            connect();

        if (dropFirst)
        {
            dropTable( _tableName);
        }
        createTable( _tableName);
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error deleting NetXpert result table first!" );
        LOGGER::LogError( ex.what() );
    }
}

void FGDBWriter::createTable ( string _tableName )
{
    currentTblPtr = unique_ptr<Table> (new Table());

    string resultTblDefStr;
    string defLine;

    //XML loading for Table Definition
    ifstream defFile(resultTblDefPath);
    while ( getline(defFile, defLine) )
    {
       resultTblDefStr.append(defLine + "\n");
    }
    defFile.close();
    //cout << resultTblDefStr << endl;
    fgdbError hr;
    wstring errorText;
    //Replace Template name "netxpert_result" with real tablename
    boost::replace_all(resultTblDefStr, "netxpert_result", _tableName);
    if ((hr = geodatabasePtr->CreateTable(resultTblDefStr,L"", *currentTblPtr) ) == S_OK)
    {
        LOGGER::LogInfo("NetXpert Result Table "+ _tableName + " created.");
    }
    else {
        ErrorInfo::GetErrorDescription(hr, errorText);
        string newErrorText;
        WStringToString(newErrorText, errorText);
        LOGGER::LogError("Error creating Result Table "+ _tableName + "!");
        LOGGER::LogError(newErrorText + " - Code: " +to_string(hr));
        throw std::runtime_error("Runtime error!");
    }

}

void FGDBWriter::openTable ( string _tableName)
{
    fgdbError hr;
    wstring errorText;

    wstring newStr;
    StringToWString(newStr, _tableName);

    if ((hr = geodatabasePtr->OpenTable(L"\\" + newStr, *currentTblPtr) ) == S_OK)
    {
        LOGGER::LogDebug("NetXpert Result Table "+ _tableName + " opened.");
    }
    else {
        ErrorInfo::GetErrorDescription(hr, errorText);
        string newErrorText;
        WStringToString(newErrorText, errorText);
        LOGGER::LogError("Error opening Result Table "+ _tableName + "!");
        LOGGER::LogError(newErrorText + " - Code: " +to_string(hr));
        throw std::runtime_error("Runtime error!");
    }
}

void FGDBWriter::dropTable (string _tableName)
{
    try
    {
        fgdbError hr;
        wstring errorText;
        wstring newTableName;
        StringToWString(newTableName, _tableName);

        if ((hr = geodatabasePtr->Delete(L"\\" + newTableName, L"Feature Class" ) ) == S_OK)
        {
            LOGGER::LogDebug("Dropped NetXpert result table first.");
        }
        else {
            ErrorInfo::GetErrorDescription(hr, errorText);
            string newErrorText;
            WStringToString(newErrorText, errorText);
            LOGGER::LogWarning("Failed to delete NetXpert result table first - maybe Feature Class does not exist!");
            LOGGER::LogWarning(newErrorText + " - Code: " +to_string(hr));
        }
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error deleting NetXpert result table first!" );
        LOGGER::LogError( ex.what() );
    }
}

void FGDBWriter::SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    const geos::geom::MultiLineString& route, string _tableName,
                                    bool truncateBeforeInsert)
{
    try
    {
        // Memory leaks:
        // Make a "local" pointer from referenced route
        //auto* route = &_route;

        string resultTableName = _tableName;

        if (truncateBeforeInsert)
            return; //TODO throw exception?

        if (!geodatabasePtr)
            connect( );

        openTable ( _tableName );

        Row row;
        currentTblPtr->CreateRowObject(row);

        wstring origNew;
        wstring destNew;
        StringToWString(origNew, orig);
        StringToWString(destNew, dest);

        row.SetString(L"fromNode", origNew);
        row.SetString(L"toNode", destNew);
        row.SetDouble(L"cost", cost);
        row.SetDouble(L"capacity", capacity);
        row.SetDouble(L"flow", flow);

        if ( !route.isEmpty() )
        {
            // Geometry
            int numPts = static_cast<int>(route.getNumPoints()); //TODO
            int numParts = static_cast<int>(route.getNumGeometries()); //TODO
            MultiPartShapeBuffer lineGeometry;

            lineGeometry.Setup(ShapeType::shapePolyline, numParts, numPts);

            // Set the point array to the array from the read geometry.
            FileGDBAPI::Point* points;
            lineGeometry.GetPoints(points);
            const geos::geom::CoordinateSequence* coordsPtr = route.getCoordinates();
            const auto& coords = *coordsPtr;
            int length = static_cast<int>(coords.getSize());
            for (int i = 0; i < length; i++)
            {
                const geos::geom::Coordinate c = coords.getAt(i);
                FileGDBAPI::Point p {c.x, c.y};
                //Point p { x = coords->getAt(i).x, y = coords->getAt(i).y };
                points[i] = p;
            }
            delete coordsPtr; //else there are memory leaks!

            // Set the parts array to the array from the read geometry.
            int* parts;
            lineGeometry.GetParts(parts);
            parts[0] = 0;
            int partNumber = 0;
            //Valgrind: memory leaks:
            // geos::geom::CoordinateArraySequenceFactory::create(std::vector<geos::geom::Coordinate,
            // std::allocator<geos::geom::Coordinate> >*, unsigned long) const (CoordinateArraySequenceFactory.inl:35
            //
            // geos::geom::CoordinateArraySequence::clone() const (CoordinateArraySequence.cpp:77
            for (int i = 1; i < numParts; i++)
            {
                const geos::geom::Geometry* geoPartPtr = route.getGeometryN(static_cast<size_t>(i - 1));
                //get position of last coordinate in part
                const auto coordsPtr = geoPartPtr->getCoordinates();
                //delete geoPart;
                partNumber = static_cast<int>(coordsPtr->getSize());
                //add number to last position
                parts[i] = partNumber + parts[i - 1];
                delete coordsPtr; //delete only this pointer! not geoPart!
            }

            lineGeometry.CalculateExtent();

            fgdbError hr;
            wstring errorText;

            hr = row.SetGeometry( lineGeometry );

            if ((hr = currentTblPtr->Insert(row)) == S_OK)
            {
                LOGGER::LogDebug("Inserted row successfully.");
            }
            else {
                ErrorInfo::GetErrorDescription(hr, errorText);
                string newErrorText;
                WStringToString(newErrorText, errorText);
                LOGGER::LogError("Error inserting row into Result Table "+ _tableName + "!");
                LOGGER::LogError(newErrorText + " - Code: " +to_string(hr));
            }
            //delete route;
        }

    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error saving results to NetXpert FileGDB!" );
        LOGGER::LogError( ex.what() );
    }

}
/*virtual void FGDBWriter::SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    geos::geom::MultiLineString route, string _tableName,
                                    bool truncateBeforeInsert, SQLiteCommand cmd);*/
void FGDBWriter::CloseConnection()
{
    try
    {
        fgdbError hr;
        wstring errorText;

        if ((hr = CloseGeodatabase( *geodatabasePtr ) ) == S_OK)
        {
            LOGGER::LogDebug("Connection to NetXpert FileGDB closed.");
        }
        else {
            ErrorInfo::GetErrorDescription(hr, errorText);
            string newErrorText;
            WStringToString(newErrorText, errorText);
            LOGGER::LogError("Error closing connection to NetXpert FileGDB!");
            LOGGER::LogError(newErrorText + " - Code: " +to_string(hr));
        }
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error closing connection to NetXpert FileGDB!" );
        LOGGER::LogError( ex.what() );
    }
}
