#ifndef SPATIALITEWRITER_H
#define SPATIALITEWRITER_H

#include <string>
#include "dbwriter.h"
#include "logger.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Transaction.h>
#include <boost/filesystem.hpp>
#include <geos/io/WKBWriter.h>

using namespace std;
using namespace geos::geom;
using namespace SQLite;

namespace NetXpert {

    /**
    * \Class that writes the result of NetXpert into a SpatiaLite DB
    **/
    class SpatiaLiteWriter : public DBWriter
    {
        public:
            SpatiaLiteWriter(Config& cnfg);
            virtual ~SpatiaLiteWriter();
            virtual void CommitCurrentTransaction();
            virtual void CreateNetXpertDB();
            virtual void CreateSolverResultTable(const string _tableName);
            virtual void CreateSolverResultTable(const string _tableName, bool dropFirst);
            virtual void OpenNewTransaction();
            SQLite::Statement* PrepareSaveSolveQueryToDB(string _tableName);
            void SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    const Geometry& route, string _tableName,
                                    bool truncateBeforeInsert, SQLite::Statement& query);
            virtual void CloseConnection();
            //string ConnStr;
        private:
            SQLite::Database* connPtr;
            SQLite::Transaction* currentTransactionPtr;
            void connect( );
            bool isConnected = false;
            void initSpatialMetaData(SQLite::Database& db);
            bool performInitialCommand(SQLite::Database& db);
            void createTable( string _tableName);
            void dropTable ( string _tableName);
            void recoverGeometryColumn (string _tableName, string _geomColName, string _geomType);
            Config NETXPERT_CNFG;
    };
}

#endif // SPATIALITEWRITER_H
