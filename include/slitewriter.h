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
            SpatiaLiteWriter(Config& cnfg, std::string dbPath);
            virtual ~SpatiaLiteWriter();
            virtual void CommitCurrentTransaction();
            virtual void CreateNetXpertDB();
            virtual void CreateSolverResultTable(const std::string& _tableName);
            virtual void CreateSolverResultTable(const std::string& _tableName, bool dropFirst);
            virtual void OpenNewTransaction();
            std::unique_ptr<SQLite::Statement> PrepareSaveResultArc(const std::string& _tableName);
            /**
            * \Brief For saving the result arc in the netXpert result DB
            * This method simply saves the data of a result arc into the netXpert result DB.
            * All result arc data is processed before calling this method.
            */
            void SaveResultArc(std::string orig, std::string dest, double cost, double capacity, double flow,
                                    const Geometry& route, std::string _tableName, SQLite::Statement& query);

            std::unique_ptr<SQLite::Statement> PrepareMergeAndSaveResultArcs(std::string arcTableName);

            /**
            * \Brief For saving a subset of original arcs in the original netXpert DB.
            * This method performs a SELECT of the arcs (arcIDs) in the original netXpert DB and inserts them
            * in a new table (data processing within the DB).
            * Obviously this is faster and memory friendlier than reading the arcs into memory first and
            * push them into a database afterwards (geometry parsing etc.).
            */
            void MergeAndSaveResultArcs(std::string orig, std::string dest, double cost, double capacity, double flow,
                                        const string geomColumnName, const string arcIDColumnName,
                                        const string arcTableName, const string& arcIDs, const string resultTableName);

            /**
            * \Brief For saving a subset of original arcs and addintional route parts in the original netXpert DB.
            * This method performs a SELECT of the arcs (arcIDs) in the original netXpert DB and inserts them
            * in a new table (data processing within the DB).
            * Obviously this is faster and memory friendlier than reading the arcs into memory first and
            * push them into a database afterwards (geometry parsing etc.).
            */
            void MergeAndSaveResultArcs(std::string orig, std::string dest, double cost, double capacity, double flow,
                                        const std::string geomColumnName, const std::string arcIDColumnName,
                                        const std::string arcTableName, const std::string& arcIDs, const MultiLineString& mLine,
                                        const std::string resultTableName);

            virtual void CloseConnection();
            //string ConnStr;
        private:
            std::string dbPath;
            std::unique_ptr<SQLite::Database> connPtr;
            std::unique_ptr<SQLite::Transaction> currentTransactionPtr;
            void connect();
            bool isConnected = false;
            void initSpatialMetaData();
            bool performInitialCommand();
            void createTable( std::string _tableName);
            void dropTable ( std::string _tableName);
            void recoverGeometryColumn (std::string _tableName, std::string _geomColName, std::string _geomType);
            Config NETXPERT_CNFG;
            void mergeAndSaveResultArcs(std::string orig, std::string dest, double cost, double capacity, double flow,
                                        std::string geomColumnName, std::string arcIDColumnName, std::string arcTableName,
                                        const std::string& arcIDs, const MultiLineString& mLine, std::string resultTableName);
            void mergeAndSaveResultArcs(std::string orig, std::string dest, double cost, double capacity, double flow,
                                        std::string geomColumnName, std::string arcIDColumnName, std::string arcTableName,
                                     const std::string& arcIDs, std::string resultTableName);
    };
}

#endif // SPATIALITEWRITER_H
