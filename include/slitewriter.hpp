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

#include <string>

#include "dbwriter.hpp"
#include "logger.hpp"

#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Transaction.h"
#include "geos/io/WKBWriter.h"

namespace netxpert {

    namespace io {
    /**
    * \brief Writes the result of netxpert into a SpatiaLite DB
    **/
    class SpatiaLiteWriter : public netxpert::io::DBWriter
    {
        public:
            ///\brief Constructor
            SpatiaLiteWriter(netxpert::cnfg::Config& cnfg);
            ///\brief Constructor with path to the database
            SpatiaLiteWriter(netxpert::cnfg::Config& cnfg, std::string dbPath);
            ///\brief Destructor
            ~SpatiaLiteWriter();
            ///\brief Commit the current transaction
            void CommitCurrentTransaction();
            ///\brief Creates the netxpert result database
            void CreateNetXpertDB();
            ///\brief Creates the solver result table, but drops the table first if it exists
            void CreateSolverResultTable(const std::string& _tableName,
                                                 const netxpert::data::NetXpertSolver solverType,
                                                 bool dropFirst = false);
            ///\brief Opens a new transaction
            void OpenNewTransaction();

            ///\brief Prepare SaveResultArc() in database
            std::unique_ptr<SQLite::Statement> PrepareSaveResultArc(const std::string& _tableName,
                                                                    const netxpert::data::NetXpertSolver solverType);

            /**
            * \brief For saving the result arc in the netXpert result DB
            * This method simply saves the data of a result arc into the netXpert result DB.
            * All result arc data is processed before
            * calling this method.
            * Save single MCFP/TP result arc
            **/
            void SaveResultArc(const std::string& orig, const std::string& dest, const double cost,
                               const double capacity, const double flow, const geos::geom::MultiLineString& route,
                               const std::string& _tableName, SQLite::Statement& query);
            ///\brief Save single SPT/ODM result arc
            void SaveResultArc(const std::string& orig, const std::string& dest, const double cost,
                                     const geos::geom::MultiLineString& route,
                                     const std::string& _tableName, SQLite::Statement& query);
            ///\brief Save single Isolines result arc
            void SaveResultArc(const std::string& orig, const double cost,
                                     const double cutoff,
                                     const geos::geom::MultiLineString& route,
                                     const std::string& _tableName, SQLite::Statement& query);
            ///\brief Save single MST result arc
            void SaveResultArc(const std::string& extArcID, const double cost,
                                     const geos::geom::MultiLineString& route,
                                     const std::string& _tableName, SQLite::Statement& query);

            ///\brief For saving the result arc of a built network into the netXpert result DB
            void SaveNetworkBuilderArc(const std::string& extArcID, const uint32_t fromNode,
                                       const uint32_t toNode, const double cost,
                                       const double capacity, const std::string& oneway,
                                       const geos::geom::Geometry& arc,
                                       const std::string& _tableName,
                                       SQLite::Statement& query);

            std::unique_ptr<SQLite::Statement> PrepareMergeAndSaveResultArcs(std::string arcTableName);

            /**
            * \brief For saving a subset of original arcs in the original netXpert DB.
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
              \warning DON'T Use me! Use Preloaded Geometries via Network::ProcessResultArcsMem() Methods

            * \brief For saving a subset of original arcs and addintional route parts in the original netXpert DB.
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
            ///\brief Closes SpatiaLite connection
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
