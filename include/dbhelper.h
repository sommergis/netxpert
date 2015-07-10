#ifndef DBHELPER_H
#define DBHELPER_H

#include <string>
#include "data.h"
#include "logger.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Transaction.h>
#include <boost/filesystem.hpp>
#include <geos/io/WKBReader.h>
#include <geos/io/WKBWriter.h>
#include <geos/geom/Point.h>
#include <geos/geom/PrecisionModel.h>

using namespace std;
using namespace geos::geom;
using namespace SQLite;


namespace netxpert
{
    /**
    * \Static Static Class that controls the SpatiaLite DB processing access
    **/
    class DBHELPER
    {
        protected:
             DBHELPER(){}
        public:
            static void Initialize(const Config& cnfg);
            static void CommitCurrentTransaction();
            static void OpenNewTransaction();
            static unique_ptr<SQLite::Statement> PrepareGetClosestArcQuery(string tableName,
                                        string geomColumnName, const ColumnMap& cmap, ArcIDColumnDataType arcIDColDataType,
                                        bool withCapacity);
            static ExtClosestArcAndPoint GetClosestArcFromPoint(Coordinate coord, int treshold,
                                            SQLite::Statement& qry, bool withCapacity);
            static InputArcs LoadNetworkFromDB(string _tableName, const ColumnMap& _map);
            static vector<NewNode> LoadNodesFromDB(string _tableName, string geomColName, const ColumnMap& _map);

            static unique_ptr<MultiLineString> GetArcGeometriesFromDB(string tableName, string arcIDColumnName,
                                        string geomColumnName, ArcIDColumnDataType arcIDColDataType, const string& arcIDs );

            //UNUSED -->
            static unique_ptr<SQLite::Statement> PrepareIsPointOnArcQuery(string tableName, string arcIDColumnName,
                                        string geomColumnName, ArcIDColumnDataType arcIDColDataType );
            static bool IsPointOnArc(Coordinate coords, string extArcID, shared_ptr<SQLite::Statement> qry);
            static double GetPositionOfClosestPoint(string arcsGeomColumnName, string arcsTableName, Coordinate coord,
                                                    string extArcID, SQLite::Statement& posOfClosestPointQry);
            //-> //
            static void CloseConnection();
            static bool IsInitialized;
            static unordered_set<string> EliminatedArcs;
            static shared_ptr<GeometryFactory> GEO_FACTORY;
            ~DBHELPER();
        private:
            static Config NETXPERT_CNFG;
            static unique_ptr<SQLite::Database> connPtr;
            static unique_ptr<SQLite::Transaction> currentTransactionPtr;
            static void connect();
            static bool isConnected;
            static bool performInitialCommand();
    };
}
#endif // DBHELPER_H
