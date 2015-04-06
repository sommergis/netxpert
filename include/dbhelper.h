#ifndef DBHELPER_H
#define DBHELPER_H

#include <string>
#include "data.h"
#include "logger.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Transaction.h>
#include <boost/filesystem.hpp>
#include <geos/io/WKBReader.h>
#include <geos/geom/Point.h>
#include <geos/geom/PrecisionModel.h>

using namespace std;
using namespace geos::geom;
using namespace SQLite;


namespace NetXpert
{
    /**
    * \Static Class that controls the SpatiaLite DB processing access for NetXpert
    **/
    class DBHELPER
    {
        protected:
             DBHELPER(){}
        public:
            static void Initialize(const Config& cnfg);
            static void CommitCurrentTransaction();
            static void OpenNewTransaction();
            static std::unique_ptr<SQLite::Statement> PrepareGetClosestArcQuery(string tableName,
                                        string geomColumnName, const ColumnMap& cmap, ArcIDColumnDataType arcIDColDataType,
                                        bool withCapacity);
            static ExtClosestArcAndPoint GetClosestArcFromPoint(Coordinate coord, int treshold,
                                            SQLite::Statement& qry, bool withCapacity);
            static InputArcs LoadNetworkFromDB(string _tableName, const ColumnMap& _map);
            static InputNodes LoadNodesFromDB(string _tableName, const ColumnMap& _map);

            //UNUSED -->
            static std::unique_ptr<SQLite::Statement> PrepareIsPointOnArcQuery(string tableName, string arcIDColumnName,
                                        string geomColumnName, ArcIDColumnDataType arcIDColDataType );
            static bool IsPointOnArc(Coordinate coords, string extArcID, SQLite::Statement& qry);
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
            static SQLite::Database* connPtr;
            static SQLite::Transaction* currentTransactionPtr;
            static void connect( );
            static bool isConnected;
            static bool performInitialCommand(SQLite::Database& db);
    };
}
#endif // DBHELPER_H
