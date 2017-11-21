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

#ifndef FGDBWRITER_H
#define FGDBWRITER_H

#include <string>
#include "dbwriter.h"
#include "logger.h"
#include <fstream>
#include "FileGDB_API/include/FileGDBAPI.h"
#include <algorithm>

namespace netxpert {
    /**
    * \brief Input/Output of netXpert
    **/
    namespace io {
    /**
    * \brief Writes the result of NetXpert into a ESRI FileGeodatabase
    **/
    class FGDBWriter : public netxpert::io::DBWriter
    {
        public:
            ///\brief Constructor
            FGDBWriter( netxpert::cnfg::Config& netxpertConfig );
            ///\brief Destructor
            ~FGDBWriter();
            ///\brief Commits the current transaction
            void CommitCurrentTransaction();
            ///\brief Creates the netxpert result database
            void CreateNetXpertDB();
            ///\brief Creates the solver result table. If the table exists it will not be overwritten
            void CreateSolverResultTable(const std::string& _tableName,
                                                 const netxpert::data::NetXpertSolver solverType);
            ///\brief Creates the solver result table, but drops the table first if it exists
            void CreateSolverResultTable(const std::string& _tableName,
                                                 const netxpert::data::NetXpertSolver solverType,
                                                 const bool dropFirst);
            ///\brief Opens a new transaction
            void OpenNewTransaction();

            /**
            * \brief For saving the result arc in the netXpert result DB
            * This method simply saves the data of a result arc into the netXpert result DB.
            * All result arc data is processed before
            * calling this method.
            * Save single MCFP/TP result arc
            **/
            void SaveResultArc(const std::string& orig, const std::string& dest, const double cost,
                               const double capacity, const double flow, const geos::geom::MultiLineString& route,
                               const std::string& _tableName);
            ///\brief Save single SPT/ODM result arc
            void SaveResultArc(const std::string& orig, const std::string& dest, const double cost,
                               const geos::geom::MultiLineString& route,
                               const std::string& _tableName);
            ///\brief Save single MST result arc
            void SaveResultArc(const std::string& originalArcID, const double cost,
                               const geos::geom::MultiLineString& route,
                               const std::string& _tableName);
            ///\brief Save single Isolines result arc
            void SaveResultArc(const std::string& orig, const double cost,
                               const double cutoff,
                               const geos::geom::MultiLineString& route,
                               const std::string& _tableName);

            ///\brief For saving the result arc of a built network into the netXpert result DB
            void SaveNetworkBuilderArc(const std::string& extArcID, const uint32_t fromNode,
                                       const uint32_t toNode, const double cost,
                                       const double capacity, const std::string& oneway,
                                       const geos::geom::Geometry& arc,
                                       const std::string& _tableName);
            ///\brief Closes File Geodatabase connection
            void CloseConnection();

        private:
            std::unique_ptr<FileGDBAPI::Geodatabase> geodatabasePtr;
            void connect();
            bool isConnected = false;
            void createTable (const std::string& _tableName,
                              const netxpert::data::NetXpertSolver solverType);
            void openTable(const std::string& _tableName);
            void dropTable (const std::string& _tableName);
            std::unique_ptr<FileGDBAPI::Table> currentTblPtr;
            const std::string resultMCFDefPath           = "FGDB_NETXPERT_MCF_SCHEMA.XML";
            const std::string resultNetBuilderDefPath    = "FGDB_NETXPERT_NETBUILD_SCHEMA.XML";
            const std::string resultMSTDefPath           = "FGDB_NETXPERT_MST_SCHEMA.XML";
            const std::string resultIsoLinesDefPath      = "FGDB_NETXPERT_ISOLINES_SCHEMA.XML";
            const std::string resultSPTDefPath           = "FGDB_NETXPERT_SPT_SCHEMA.XML";
            netxpert::cnfg::Config NETXPERT_CNFG;
    };
} //namespace io
} //namespace netxpert


#endif // FGDBWRITER_H

