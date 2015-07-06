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

namespace netxpert {

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
            unique_ptr<SQLite::Statement> PrepareSaveSolveQueryToDB(string _tableName);
            void SaveSolveQueryToDB(string orig, string dest, double cost, double capacity, double flow,
                                    const Geometry& route, string _tableName,
                                    bool truncateBeforeInsert, SQLite::Statement& query);
            //TODO: testme
            unique_ptr<SQLite::Statement> PrepareCreateRouteGeometries(string arcTableName);
            /*void CreateRouteGeometries(string geomColumnName, string arcIDColumnName, string arcTableName,
                                     const string& arcIDs, vector<Geometry> segments, string resultTableName);*/
            void CreateRouteGeometries(const string& geomColumnName, const string& arcIDColumnName, const string& arcTableName,
                                     const string& arcIDs, const MultiLineString& mLine,
                                     const string& resultTableName);

            virtual void CloseConnection();
            //string ConnStr;
        private:
            unique_ptr<SQLite::Database> connPtr;
            unique_ptr<SQLite::Transaction> currentTransactionPtr;
            void connect( );
            bool isConnected = false;
            void initSpatialMetaData();
            bool performInitialCommand();
            void createTable( string _tableName);
            void dropTable ( string _tableName);
            void recoverGeometryColumn (string _tableName, string _geomColName, string _geomType);
            Config NETXPERT_CNFG;
            void createRouteWithAllParts(const string& geomColumnName, const string& arcIDColumnName, const string& arcTableName,
                                     const string& arcIDs, vector<shared_ptr<Geometry>> segments, const string& resultTableName);
            void createRouteWithAllParts_2(const string& geomColumnName, const string& arcIDColumnName, const string& arcTableName,
                                     const string& arcIDs, const MultiLineString& mLine, const string& resultTableName);
            void createRouteWithStartAndMiddlePart(const string& geomColumnName, const string& arcIDColumnName, const string& arcTableName,
                                     const string& arcIDs, const Geometry& geom, const string& resultTableName);
            void createRouteWithEndAndMiddlePart(const string& geomColumnName, const string& arcIDColumnName, const string& arcTableName,
                                     const string& arcIDs, const Geometry& geom, const string& resultTableName);
            void createRouteWithStartAndEnd(const string& arcTableName, vector<shared_ptr<Geometry>>segments,
                                                    const string& resultTableName);
            void createRouteWithMiddlePart(const string& geomColumnName, const string& arcIDColumnName, const string& arcTableName,
                                     const string& arcIDs, const string& resultTableName);
            void createRouteWithOnePart(const string& arcTableName, const Geometry& geom,
                                                    const string& resultTableName);
    };
}

#endif // SPATIALITEWRITER_H
