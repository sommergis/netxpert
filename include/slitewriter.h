#ifndef SPATIALITEWRITER_H
#define SPATIALITEWRITER_H

#include "utils.h"
#include <string>
#include "dbwriter.h"
#include "logger.h"
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Transaction.h"
#include "geos/io/WKBWriter.h"

using namespace std;
using namespace geos::geom;
using namespace SQLite;

namespace netxpert {

    /**
    * \Class Writes the result of NetXpert into a SpatiaLite DB
    **/
    class SpatiaLiteWriter : public DBWriter
    {
        public:
            SpatiaLiteWriter(Config& cnfg);
            virtual ~SpatiaLiteWriter();
            virtual void CommitCurrentTransaction();
            virtual void CreateNetXpertDB();
            virtual void CreateSolverResultTable(const string& _tableName);
            virtual void CreateSolverResultTable(const string& _tableName, bool dropFirst);
            virtual void OpenNewTransaction();
            unique_ptr<SQLite::Statement> PrepareSaveResultArc(string _tableName);
            void SaveResultArc(string orig, string dest, double cost, double capacity, double flow,
                                    const Geometry& route, string _tableName, SQLite::Statement& query);

            unique_ptr<SQLite::Statement> PrepareMergeAndSaveResultArcs(string arcTableName);
            void MergeAndSaveResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                        const string geomColumnName, const string arcIDColumnName,
                                        const string arcTableName, const string& arcIDs, const string resultTableName);
            void MergeAndSaveResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                        const string geomColumnName, const string arcIDColumnName,
                                        const string arcTableName, const string& arcIDs, const MultiLineString& mLine,
                                        const string resultTableName);

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
            void mergeAndSaveResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                        string geomColumnName, string arcIDColumnName, string arcTableName,
                                        const string& arcIDs, const MultiLineString& mLine, string resultTableName);
            void mergeAndSaveResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                        string geomColumnName, string arcIDColumnName, string arcTableName,
                                     const string& arcIDs, string resultTableName);
    };
}

#endif // SPATIALITEWRITER_H
