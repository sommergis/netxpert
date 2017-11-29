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

#ifndef DBHELPER_H
#define DBHELPER_H

#include <string>
//determination of type
#include <typeinfo>
//#include "utils.h"
#include "data.h"
#include "logger.h"
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Transaction.h"
#include "geos/io/WKBReader.h"
#include "geos/io/WKBWriter.h"
#include "geos/geom/Point.h"
#include "geos/geom/PrecisionModel.h"

namespace netxpert {
    /**
    * \brief Input/Output of netXpert
    **/
    namespace io {
    /**
    * \brief Static class that controls the SpatiaLite DB processing access
    **/
    class DBHELPER
    {
        protected:
            ///\brief Empty Constructor is protected because it is a static class
            DBHELPER(){}
        public:
            ///\brief Initializes the static DBHELPER and also the GEO_FACTORY and the Precision Model for all geometries.
            static void Initialize(const netxpert::cnfg::Config& cnfg);
            ///\brief Commits the current transaction
            static void CommitCurrentTransaction();
            ///\brief Opens a new database transaction
            static void OpenNewTransaction();
            ///\brief Loads arcs that form a network from the database
            static netxpert::data::InputArcs LoadNetworkFromDB(const std::string& _tableName,
                                                               const netxpert::data::ColumnMap& _map);
            ///\brief Loads arcs that shall be built (from and to nodes will be calculated) from the database
            static netxpert::data::NetworkBuilderInputArcs LoadNetworkToBuildFromDB(const std::string& _tableName,
                                                                                    const netxpert::data::ColumnMap& _map);
            ///\brief Loads all nodes from the database
            static std::vector<netxpert::data::NewNode> LoadNodesFromDB(const std::string& _tableName, const std::string& geomColName,
                                                                  const netxpert::data::ColumnMap& _map);
            ///\brief Prepares query for GetClosestArcQuery()
            static std::unique_ptr<SQLite::Statement> PrepareGetClosestArcQuery(const std::string& tableName,
                                        const std::string& geomColumnName, const netxpert::data::ColumnMap& cmap,
                                        const netxpert::data::ArcIDColumnDataType arcIDColDataType,
                                        const bool withCapacity);
            ///\brief Gets closest arc to given coordinate within the given threshold
            static netxpert::data::ExtClosestArcAndPoint GetClosestArcFromPoint(const geos::geom::Coordinate& coord,
                                                                          const int threshold, SQLite::Statement& qry,
                                                                          const bool withCapacity);
            ///\brief Gets single arc geometry from database per ID
            static std::unique_ptr<geos::geom::MultiLineString> GetArcGeometryFromDB(const std::string& tableName,
                                                             const std::string& arcIDColumnName,
                                                             const std::string& geomColumnName,
                                                             const netxpert::data::ArcIDColumnDataType arcIDColDataType,
                                                             const netxpert::data::extarcid_t& arcID);
            ///\brief Gets arc geometries from database per IDs
            static std::unique_ptr<geos::geom::MultiLineString> GetArcGeometriesFromDB(const std::string& tableName,
                                                                           const std::string& arcIDColumnName,
                                                                           const std::string& geomColumnName,
                                                                           const netxpert::data::ArcIDColumnDataType arcIDColDataType,
                                                                           const std::string& arcIDs);
            ///\brief Gets arc geometries that intersect with barrier geometries from database
            static std::unordered_set<netxpert::data::extarcid_t> GetIntersectingArcs(const std::string& barrierTableName,
                                                                       const std::string& barrierGeomColName,
                                                                       const std::string& arcsTableName,
                                                                       const std::string& arcIDColName,
                                                                       const std::string& arcGeomColName);
            ///\brief Gets barrier geometries from database
            static std::vector<std::unique_ptr<geos::geom::Geometry>> GetBarrierGeometriesFromDB(const std::string& barrierTableName,
                                                                       const std::string& barrierGeomColName);
            ///\warning untested
            static std::unique_ptr<geos::geom::MultiPoint> GetArcVertexGeometriesByBufferFromDB(const std::string& tableName,
                                                                           const std::string& geomColumnName,
                                                                           const netxpert::data::ArcIDColumnDataType arcIDColDataType,
                                                                           const std::string& arcIDColName,
                                                                           const double bufferVal,
                                                                           const geos::geom::Coordinate& p);

            /// @cond UNUSED
            static std::unique_ptr<SQLite::Statement>
            PrepareIsPointOnArcQuery(std::string tableName,
                                     std::string arcIDColumnName,
                                     std::string geomColumnName,
                                     netxpert::data::ArcIDColumnDataType arcIDColDataType );

            static bool IsPointOnArc(geos::geom::Coordinate coords,
                                     std::string extArcID,
                                     std::shared_ptr<SQLite::Statement> qry);

            static double GetPositionOfClosestPoint(std::string arcsGeomColumnName,
                                                    std::string arcsTableName,
                                                    geos::geom::Coordinate coord,
                                                    std::string extArcID,
                                                    SQLite::Statement& posOfClosestPointQry);
            ///@endcond

            ///\brief Closes the database connection
            static void CloseConnection();
            ///\brief Indicated if DBHELPER has been already initialized
            static bool IsInitialized;
            ///\brief Stores the elmininated arcs, that shall be excluded from the databse queries
            static std::unordered_set<netxpert::data::extarcid_t> EliminatedArcs;
            ///\brief Geometry Factory for GEOS.
            /// A precision model is set in the initialization of DBHELPER.
            /// Used for WKBReader and every geometry creation in netxpert library.
            static std::shared_ptr<geos::geom::GeometryFactory> GEO_FACTORY;

            /* Methods for loading IDs and geometry into a map in memory for fast access to geometries.
               Much faster (factor x10) than spatialite lookup per primary key lookup of IDs
            */
            ///\brief Fills the key value memory store, that speeds up queries for geometries
            static void LoadGeometryToMem(const std::string& _tableName, const netxpert::data::ColumnMap& _map,
                                          const std::string& geomColumnName, const std::string& arcIDs);
            ///\brief Get the geometries from the key value memory store
            static std::unique_ptr<geos::geom::MultiLineString> GetArcGeometriesFromMem(const std::string& arcIDs);

            ///\brief Destructor
            ~DBHELPER();
        private:
            static std::unordered_map<netxpert::data::extarcid_t, std::shared_ptr<geos::geom::LineString>> KV_Network;
            static netxpert::cnfg::Config NETXPERT_CNFG;
            static std::unique_ptr<SQLite::Database> connPtr;
            static std::unique_ptr<SQLite::Transaction> currentTransactionPtr;
            //static void connect();
            static void connect(bool inMemory);
            static bool isConnected;
            static bool performInitialCommand();
            static void initSpatialMetaData();
            static void optimizeSQLiteCon();
    };
} //namespace io
} //namespace netxpert

#endif // DBHELPER_H
