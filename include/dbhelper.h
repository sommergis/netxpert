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

    namespace io {
    /**
    * \Static Static Class that controls the SpatiaLite DB processing access
    **/
    class DBHELPER
    {
        protected:
             DBHELPER(){}
        public:
            static void Initialize(const netxpert::cnfg::Config& cnfg);
            static void CommitCurrentTransaction();
            static void OpenNewTransaction();
            static std::unique_ptr<SQLite::Statement> PrepareGetClosestArcQuery(const std::string& tableName,
                                        const std::string& geomColumnName, const netxpert::data::ColumnMap& cmap,
                                        const netxpert::data::ArcIDColumnDataType arcIDColDataType,
                                        const bool withCapacity);
            static netxpert::data::ExtClosestArcAndPoint GetClosestArcFromPoint(const geos::geom::Coordinate& coord,
                                                                          const int treshold, SQLite::Statement& qry,
                                                                          const bool withCapacity);
            static netxpert::data::InputArcs LoadNetworkFromDB(const std::string& _tableName,
                                                               const netxpert::data::ColumnMap& _map);
            static netxpert::data::NetworkBuilderInputArcs LoadNetworkToBuildFromDB(const std::string& _tableName,
                                                                                    const netxpert::data::ColumnMap& _map);
            static std::vector<netxpert::data::NewNode> LoadNodesFromDB(const std::string& _tableName, const std::string& geomColName,
                                                                  const netxpert::data::ColumnMap& _map);

            static std::unique_ptr<geos::geom::MultiLineString> GetArcGeometryFromDB(const std::string& tableName,
                                                             const std::string& arcIDColumnName,
                                                             const std::string& geomColumnName,
                                                             const netxpert::data::ArcIDColumnDataType arcIDColDataType,
                                                             const netxpert::data::extarcid_t& arcID);

            static std::unique_ptr<geos::geom::MultiLineString> GetArcGeometriesFromDB(const std::string& tableName,
                                                                           const std::string& arcIDColumnName,
                                                                           const std::string& geomColumnName,
                                                                           const netxpert::data::ArcIDColumnDataType arcIDColDataType,
                                                                           const std::string& arcIDs );

            static std::unordered_set<netxpert::data::extarcid_t> GetIntersectingArcs(const std::string& barrierTableName,
                                                                       const std::string& barrierGeomColName,
                                                                       const std::string& arcsTableName,
                                                                       const std::string& arcIDColName,
                                                                       const std::string& arcGeomColName);

            static std::vector<std::unique_ptr<geos::geom::Geometry>> GetBarrierGeometriesFromDB(const std::string& barrierTableName,
                                                                       const std::string& barrierGeomColName);

            static std::unique_ptr<geos::geom::MultiPoint> GetArcVertexGeometriesByBufferFromDB(const std::string& tableName,
                                                                           const std::string& geomColumnName,
                                                                           const netxpert::data::ArcIDColumnDataType arcIDColDataType,
                                                                           const std::string& arcIDColName,
                                                                           const double bufferVal,
                                                                           const geos::geom::Coordinate& p);

            //UNUSED -->
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
            //-> //

            static void CloseConnection();
            static bool IsInitialized;
            static std::unordered_set<netxpert::data::extarcid_t> EliminatedArcs;
            static std::shared_ptr<geos::geom::GeometryFactory> GEO_FACTORY;

            /* Methods for loading IDs and geometry into a map in memory for fast access to geometries.
               Much faster (factor x10) than spatialite lookup per primary key lookup of IDs
            */
            /* Fills KV_Network */
            static void LoadGeometryToMem(const std::string& _tableName, const netxpert::data::ColumnMap& _map,
                                          const std::string& geomColumnName);
            /* Fills KV_Network */
            static void LoadGeometryToMem(const std::string& _tableName, const netxpert::data::ColumnMap& _map,
                                          const std::string& geomColumnName, const std::string& arcIDs);
            static std::unique_ptr<geos::geom::MultiLineString> GetArcGeometriesFromMem(const std::string& arcIDs);

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
