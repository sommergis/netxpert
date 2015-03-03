#ifndef DBHELPER_H
#define DBHELPER_H

#include <string>
#include "data.h"
#include "logger.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Transaction.h>
#include <boost/filesystem.hpp>
#include <geos/io/WKBWriter.h>

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
             DBHELPER();
        public:
            static Config NETXPERT_CNFG;
            static void Initialize();
            static void CommitCurrentTransaction();
            static void OpenNewTransaction();
            //SQLite::Statement* PrepareSaveSolveQueryToDB(string _tableName);
            /*void SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    Geometry& route, string _tableName,
                                    bool truncateBeforeInsert, SQLite::Statement& query);*/
            static InputArcs LoadNetworkFromDB(string _tableName, ColumnMap _map);
            static InputNodes LoadNodesFromDB(string _tableName, ColumnMap _map);
            static void CloseConnection();
            ~DBHELPER();

        private:
            static SQLite::Database* connPtr;
            static SQLite::Transaction* currentTransactionPtr;
            static void connect( );
            static bool isConnected;
            static bool performInitialCommand(SQLite::Database& db);
    };
}
#endif // DBHELPER_H
