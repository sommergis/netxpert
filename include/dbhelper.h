#ifndef DBHELPER_H
#define DBHELPER_H

#include <string>
#include "utils.h"
#include "data.h"
#include "logger.h"
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Transaction.h"
#include "geos/io/WKBReader.h"
#include "geos/io/WKBWriter.h"
#include "geos/geom/Point.h"
#include "geos/geom/PrecisionModel.h"

using namespace std;
using namespace geos::geom;
using namespace SQLite;


namespace netxpert
{
    /**
    * \Static Static Class that controls the SpatiaLite DB processing access
    **/
    class DBHELPER
    {
        protected:
             DBHELPER(){}
        public:
            static void Initialize(const netxpert::Config& cnfg);
            static void CommitCurrentTransaction();
            static void OpenNewTransaction();
            static std::unique_ptr<SQLite::Statement> PrepareGetClosestArcQuery(const std::string& tableName,
                                        const std::string& geomColumnName, const ColumnMap& cmap,
                                        const netxpert::ArcIDColumnDataType arcIDColDataType,
                                        const bool withCapacity);
            static netxpert::ExtClosestArcAndPoint GetClosestArcFromPoint(const geos::geom::Coordinate& coord,
                                                                          const int treshold, SQLite::Statement& qry,
                                                                          const bool withCapacity);
            static netxpert::InputArcs LoadNetworkFromDB(const std::string& _tableName, const ColumnMap& _map);
            static netxpert::NetworkBuilderInputArcs LoadNetworkToBuildFromDB(const std::string& _tableName, const ColumnMap& _map);
            static std::vector<netxpert::NewNode> LoadNodesFromDB(const std::string& _tableName, const std::string& geomColName,
                                                                  const ColumnMap& _map);

            static std::unique_ptr<geos::geom::MultiLineString> GetArcGeometriesFromDB(const std::string& tableName,
                                                                           const std::string& arcIDColumnName,
                                                                           const std::string& geomColumnName,
                                                                           const ArcIDColumnDataType arcIDColDataType,
                                                                           const std::string& arcIDs );

            static std::unordered_set<std::string> GetIntersectingArcs(const std::string& barrierTableName,
                                                                       const std::string& barrierGeomColName,
                                                                       const std::string& arcsTableName,
                                                                       const std::string& arcIDColName,
                                                                       const std::string& arcGeomColName);

            //UNUSED -->
            static std::unique_ptr<SQLite::Statement> PrepareIsPointOnArcQuery(std::string tableName, std::string arcIDColumnName,
                                        std::string geomColumnName, ArcIDColumnDataType arcIDColDataType );
            static bool IsPointOnArc(Coordinate coords, std::string extArcID, std::shared_ptr<SQLite::Statement> qry);
            static double GetPositionOfClosestPoint(std::string arcsGeomColumnName, std::string arcsTableName, Coordinate coord,
                                                    std::string extArcID, SQLite::Statement& posOfClosestPointQry);
            //-> //

            static void CloseConnection();
            static bool IsInitialized;
            static std::unordered_set<std::string> EliminatedArcs;
            static std::shared_ptr<geos::geom::GeometryFactory> GEO_FACTORY;

            /* Methods for loading IDs and geometry into a map in memory for fast access to geometries.
               Much faster (factor x10) than spatialite lookup per primary key lookup of IDs
            */
            /* Fills KV_Network */
            static void LoadGeometryToMem(const std::string& _tableName, const ColumnMap& _map,
                                          const std::string& geomColumnName);
            /* Fills KV_Network */
            static void LoadGeometryToMem(const std::string& _tableName, const ColumnMap& _map,
                                          const std::string& geomColumnName, const std::string& arcIDs);
            static std::unique_ptr<geos::geom::MultiLineString> GetArcGeometriesFromMem(const std::string& arcIDs);

            ~DBHELPER();
        private:
            static std::unordered_map<std::string, std::shared_ptr<geos::geom::LineString>> KV_Network;
            static netxpert::Config NETXPERT_CNFG;
            static std::unique_ptr<SQLite::Database> connPtr;
            static std::unique_ptr<SQLite::Transaction> currentTransactionPtr;
            //static void connect();
            static void connect(bool inMemory);
            static bool isConnected;
            static bool performInitialCommand();
            static void initSpatialMetaData();
            static void optimizeSQLiteCon();
    };
}
#endif // DBHELPER_H
