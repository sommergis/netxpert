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
            static void Initialize(Config& cnfg);
            static void CommitCurrentTransaction();
            static void OpenNewTransaction();
            static std::unique_ptr<SQLite::Statement> PrepareGetClosestArcQuery(string tableName, string arcIDColName,
                            string geomColName, ArcIDColumnDataType arcIDColDataType);
            static ClosestArcAndPoint GetClosestArcFromPoint(Coordinate coord, int treshold,
                                            SQLite::Statement& qry);
            static std::unique_ptr<SQLite::Statement> PrepareIsPointOnArcQuery(string tableName, string arcIDColumnName,
                                        string geomColumnName, ArcIDColumnDataType arcIDColDataType );
            static bool IsPointOnArc(Coordinate coords, string arcID, SQLite::Statement& qry);
            static InputArcs LoadNetworkFromDB(string _tableName, ColumnMap _map);
            static InputNodes LoadNodesFromDB(string _tableName, ColumnMap _map);
            static void CloseConnection();
            static bool IsInitialized;
            static unordered_set<string> EliminatedArcs;
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
