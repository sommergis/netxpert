/*
 * This file is a part of netxpert.
 *
 * Copyright (C) 2013-2017
 * Johannes Sommer, Christopher Koller
 *
 * Permission to use, modify and distribute this software is granted
 * provided that this copyright notice appears in all copies. For
 * precise terms see the accompanying LICENSE file.
 *
 * This software is provided "AS IS" with no warranty of any kind,
 * express or implied, and with no claim as to its suitability for any
 * purpose.
 *
 */

#ifndef SPATIALITEWRITER_H
#define SPATIALITEWRITER_H

//#include "utils.h"
#include <string>
#include "dbwriter.h"
#include "logger.h"
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Transaction.h"
#include "geos/io/WKBWriter.h"

namespace netxpert {

    namespace io {
    /**
    * \Class Writes the result of NetXpert into a SpatiaLite DB
    **/
    class SpatiaLiteWriter : public netxpert::io::DBWriter
    {
        public:
            SpatiaLiteWriter(netxpert::cnfg::Config& cnfg);
            SpatiaLiteWriter(netxpert::cnfg::Config& cnfg, std::string dbPath);
            ~SpatiaLiteWriter();
            void CommitCurrentTransaction();
            void CreateNetXpertDB();
            void CreateSolverResultTable(const std::string& _tableName,
                                                 const netxpert::data::NetXpertSolver solverType);
            void CreateSolverResultTable(const std::string& _tableName,
                                                 const netxpert::data::NetXpertSolver solverType,
                                                 const bool dropFirst);
            void OpenNewTransaction();


            std::unique_ptr<SQLite::Statement> PrepareSaveResultArc(const std::string& _tableName,
                                                                    const netxpert::data::NetXpertSolver solverType);

            /**
            * \Brief For saving the result arc in the netXpert result DB
            * This method simply saves the data of a result arc into the netXpert result DB.
            * All result arc data is processed before calling this method.
            */
            //MCF, TP
            void SaveResultArc(const std::string& orig, const std::string& dest, const double cost,
                               const double capacity, const double flow, const geos::geom::MultiLineString& route,
                               const std::string& _tableName, SQLite::Statement& query);
            //SPT, ODM
            void SaveResultArc(const std::string& orig, const std::string& dest, const double cost,
                                     const geos::geom::MultiLineString& route,
                                     const std::string& _tableName, SQLite::Statement& query);
            //Isolines
            void SaveResultArc(const std::string& orig, const double cost,
                                     const double cutoff,
                                     const geos::geom::MultiLineString& route,
                                     const std::string& _tableName, SQLite::Statement& query);
            //MST
            void SaveResultArc(const std::string& extArcID, const double cost,
                                     const geos::geom::MultiLineString& route,
                                     const std::string& _tableName, SQLite::Statement& query);

            /**
            * \Brief For saving the result arc of a built network into the netXpert result DB
            */
            void SaveNetworkBuilderArc(const std::string& extArcID, const uint32_t fromNode,
                                       const uint32_t toNode, const double cost,
                                       const double capacity, const std::string& oneway,
                                       const geos::geom::Geometry& arc,
                                       const std::string& _tableName,
                                       SQLite::Statement& query);

            std::unique_ptr<SQLite::Statement> PrepareMergeAndSaveResultArcs(std::string arcTableName);

            /**
            * \Brief For saving a subset of original arcs in the original netXpert DB.
            * This method performs a SELECT of the arcs (arcIDs) in the original netXpert DB and inserts them
            * in a new table (data processing within the DB).
            * Obviously this is faster and memory friendlier than reading the arcs into memory first and
            * push them into a database afterwards (geometry parsing etc.).
            */
            void MergeAndSaveResultArcs(const std::string& costColumnName, const std::string& geomColumnName,
                                        const std::string arcIDColumnName,
                                        const std::string& arcTableName, const std::string& arcIDs,
                                        const std::string& resultTableName);

            /**
              \Warn DON'T Use me! Use Preloaded Geometries via Network::ProcessResultArcsMem() Methods

            * \Brief For saving a subset of original arcs and addintional route parts in the original netXpert DB.
            * This method performs a SELECT of the arcs (arcIDs) in the original netXpert DB and inserts them
            * in a new table (data processing within the DB).
            * Obviously this is faster and memory friendlier than reading the arcs into memory first and
            * push them into a database afterwards (geometry parsing etc.).
            */
            void MergeAndSaveResultArcs(const std::string& orig, const std::string& dest, const double cost,
                                        const double capacity, const double flow,
                                        const std::string& geomColumnName, const std::string& arcIDColumnName,
                                        const std::string& arcTableName, const std::string& arcIDs,
                                        const geos::geom::MultiLineString& mLine,
                                        const std::string& resultTableName);

            void CloseConnection();
            //string ConnStr;
        private:
            std::string dbPath;
            std::unique_ptr<SQLite::Database> connPtr;
            std::unique_ptr<SQLite::Transaction> currentTransactionPtr;
            void connect();
            bool isConnected = false;
            void initSpatialMetaData();
            bool performInitialCommand();
            void createTable (const std::string& _tableName, const netxpert::data::NetXpertSolver solverType);
            void dropTable (const std::string& _tableName);
            void recoverGeometryColumn (std::string _tableName, std::string _geomColName, std::string _geomType);
            netxpert::cnfg::Config NETXPERT_CNFG;
            //UNUSED
            void mergeAndSaveResultArcs(std::string orig, std::string dest, double cost, double capacity, double flow,
                                        std::string geomColumnName, std::string arcIDColumnName, std::string arcTableName,
                                        const std::string& arcIDs, const geos::geom::MultiLineString& mLine, std::string resultTableName);

            void mergeAndSaveResultArcs(std::string costColumnName, std::string geomColumnName, std::string arcIDColumnName,
                                        std::string arcTableName, const std::string& arcIDs, std::string resultTableName);
    };
} //namespace io
} //namespace netxpert

#endif // SPATIALITEWRITER_H
