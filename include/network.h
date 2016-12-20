#ifndef NETWORK_H
#define NETWORK_H

#include <list>
#include "utils.h"
#include "dbhelper.h"
#include "slitewriter.h"
#include "fgdbwriter.h"
#include <algorithm>
#include "geos/geom/CoordinateSequenceFactory.h"
#include "geos/geom/GeometryFactory.h"
#include "geos/opDistance.h"
//#include "geos/index/strtree/STRtree.h"
#include "geos/linearref/LengthIndexedLine.h"
#include "geos/geom/LineString.h"
#include "geos/linearref/ExtractLineByLocation.h"
#include "geos/linearref/LengthIndexOfPoint.h"
#include "geos/linearref/LocationIndexOfLine.h"
#include "geos/linearref/LocationIndexedLine.h"
#include "geos/linearref/LengthLocationMap.h"
#include "geos/io/WKTReader.h"
#include "geos/opLinemerge.h"
#include <cmath>

namespace netxpert
{
    /**
    * \Class Stores the representation of the network and provides methods for I/O of the nodes and arcs.
    **/
    class Network
    {
        public:
            /**
            * Default constructor
            * Handles only arc input (which can be enough for some solvers - e.g. Minimum Spanning Tree).
            * Changes to the network can be made afterwards calling LoadStartNodes() and LoadEndNodes().
            **/
            Network(const netxpert::data::InputArcs& arcsTbl,
                    const netxpert::data::ColumnMap& _map,
                    const netxpert::cnfg::Config& cnfg);
            /**
            * Handles both arcs and nodes from the constructor call which can carry positive supply or negative demand values.
            **/
            Network(const netxpert::data::InputArcs& arcsTbl,
                    const netxpert::data::InputNodes& nodesTbl,
                    const netxpert::data::ColumnMap& _map,
                    const netxpert::cnfg::Config& cnfg);
            /**
            * Minimal constructor for the network.
            * TESTME
            **/
            Network(netxpert::data::Arcs arcData,
                    std::unordered_map<netxpert::data::ExtArcID,
                    netxpert::data::IntNodeID> distinctNodeIDs,
                    netxpert::data::NodeSupplies _nodeSupplies);

            /**
            * Simple Interface for adding start nodes
            */
            unsigned int AddStartNode(std::string extArcID,
                                      double x, double y, double supply,
                                      int treshold,
                                      const netxpert::data::ColumnMap& cmap,
                                      bool withCapacity);
            /**
            * Simple Interface for adding end nodes
            */
            unsigned int AddEndNode(std::string extArcID,
                                    double x, double y, double supply,
                                    int treshold,
                                    const netxpert::data::ColumnMap& cmap,
                                    bool withCapacity);

            unsigned int AddStartNode(const netxpert::data::NewNode& newNode,
                                      const int treshold,
                                      SQLite::Statement& closestArcQry,
                                      const bool withCapacity);

            unsigned int AddEndNode(const netxpert::data::NewNode& newNode,
                                    const int treshold,
                                    SQLite::Statement& closestArcQry,
                                    const bool withCapacity);

            std::vector< std::pair<unsigned int, std::string> > LoadStartNodes(const std::vector<netxpert::data::NewNode>& newNodes, const int treshold,
                                                                const std::string arcsTableName, const std::string geomColumnName,
                                                                const netxpert::data::ColumnMap& cmap, const bool withCapacity);
            std::vector< std::pair<unsigned int, std::string> > LoadEndNodes(const std::vector<netxpert::data::NewNode>& newNodes, const int treshold,
                                                                const std::string arcsTableName, const std::string geomColumnName,
                                                                const netxpert::data::ColumnMap& cmap, const bool withCapacity);

            /** Method for processing and saving a subset of original arcs as results (e.g. MST) */
            void ProcessResultArcs(/*const std::string& orig, const std::string& dest,  const double cost,
                                   const double capacity, const double flow, */
                                   const std::string& arcIDs,
                                   const std::string& resultTableName);

            /** For Testing purposes only */
            void ProcessResultArcs(const std::string& orig, const std::string& dest, const double cost,
                                   const double capacity, const double flow,
                                   const std::string& arcIDs, std::vector<netxpert::data::InternalArc>& routeNodeArcRep,
                                   const std::string& resultTableName, netxpert::io::DBWriter& writer );

            /** Main method for processing and saving result arcs */
            void ProcessResultArcs(const std::string& orig, const std::string& dest, const double cost,
                                   const double capacity, const double flow,
                                   const std::string& arcIDs, std::vector<netxpert::data::InternalArc>& routeNodeArcRep,
                                   const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                   SQLite::Statement& qry //can be null in case of ESRI FileGDB
                                   );
            /** Main method for processing and saving result arcs (preloading geometry into memory)
                Solver: MCF, TPs
                */
            void ProcessMCFResultArcsMem(const std::string& orig, const std::string& dest, const double cost,
                                        const double capacity, const double flow,
                                        const std::string& arcIDs, std::vector<netxpert::data::InternalArc>& routeNodeArcRep,
                                        const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                        SQLite::Statement& qry //can be null in case of ESRI FileGDB
                                        );
            /** Main method for processing and saving result arcs (preloading geometry into memory)
                Solver: SPT, ODM
                */
            void ProcessSPTResultArcsMem(const std::string& orig, const std::string& dest, const double cost,
                                        const std::string& arcIDs, std::vector<netxpert::data::InternalArc>& routeNodeArcRep,
                                        const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                        SQLite::Statement& qry //can be null in case of ESRI FileGDB
                                        );

            /** Main method for processing and saving result arcs (preloading geometry into memory)
                Solver: Isolines
                */
            void ProcessIsoResultArcsMem(const std::string& orig, const double cost,
                                        const std::string& arcIDs, std::vector<netxpert::data::InternalArc>& routeNodeArcRep,
                                        const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                        SQLite::Statement& qry, //can be null in case of ESRI FileGDB
                                        const std::unordered_map<netxpert::data::ExtNodeID,
                                        std::vector<double> > cutOffs
                                        );

            void ConvertInputNetwork(bool autoClean);

            //Network Helpers
            netxpert::data::SplittedArc GetSplittedClosestNewArcToPoint(const geos::geom::Coordinate& coord,
                                                                  const int treshold);

            netxpert::data::SplittedArc GetSplittedClosestOldArcToPoint(const geos::geom::Coordinate coord, const int treshold,
                                                        const std::pair<netxpert::data::ExternalArc,netxpert::data::ArcData>& arcData,
                                                        const geos::geom::Geometry& arc);

            bool IsPointOnArc(const geos::geom::Coordinate& coords, const geos::geom::Geometry& arc);

            //Helpers for looking up original data
            std::unordered_set<std::string> GetOriginalArcIDs(const std::vector<netxpert::data::InternalArc>& ftNodes,
                                                            const bool isDirected) const;
            std::vector<netxpert::data::ArcData> GetOriginalArcData(const std::vector<netxpert::data::InternalArc>& ftNodes,
                                                    const bool isDirected) const;
            //TODO
            void GetOriginalArcDataAndFlow(const std::list<netxpert::data::ArcDataAndFlow>& origArcDataAndFlow,
                                            const std::list<netxpert::data::InternalArc>& startEndNodes,
                                            const bool isDirected);

            std::string GetOriginalNodeID(const unsigned int internalNodeID);
            unsigned int GetInternalNodeID(const std::string& externalNodeID);
            std::string GetOriginalStartOrEndNodeID(const unsigned int internalNodeID);
            geos::geom::Coordinate GetStartOrEndNodeGeometry(const unsigned int internalNodeID);
            geos::geom::Coordinate GetStartOrEndNodeGeometry(const std::string& externalNodeID);
            double GetPositionOfPointAlongLine(const geos::geom::Coordinate& coord,
                                                const geos::geom::Geometry& arc);
            netxpert::data::StartOrEndLocationOfLine GetLocationOfPointOnLine(const geos::geom::Coordinate& coord,
                                                                const geos::geom::Geometry& arc);

            unsigned int GetMaxNodeCount();
            unsigned int GetMaxArcCount();
            unsigned int GetCurrentNodeCount();
            unsigned int GetCurrentArcCount();
            netxpert::data::Arcs& GetInternalArcData();
            netxpert::data::NodeSupplies GetNodeSupplies();
            netxpert::data::Arcs& GetOldArcs();
            netxpert::data::NewArcs& GetNewArcs();

            void Reset();

            //MCF
            netxpert::data::MinCostFlowInstanceType GetMinCostFlowInstanceType();
            void TransformUnbalancedMCF(netxpert::data::MinCostFlowInstanceType mcfInstanceType);

            //DIMACS
            void ExportToDIMACS(const std::string& path);

            virtual ~Network();

        private:
            std::vector<netxpert::data::InternalArc> insertNewStartNode(const bool isDirected, netxpert::data::SplittedArc& splittedLine,
                                                        const std::string& extArcID, const std::string& extNodeID,
                                                        const geos::geom::Coordinate& startPoint);
            std::vector<netxpert::data::InternalArc> insertNewEndNode(const bool isDirected, netxpert::data::SplittedArc& splittedLine,
                                                 const std::string& extArcID, const std::string& extNodeID,
                                                 const geos::geom::Coordinate& endPoint);
            void renameNodes();
            void readNetworkFromTable(const bool autoClean, const bool oneWay);
            void processArc(const netxpert::data::InputArc& arc, const unsigned int internalStartNode,
                            const unsigned int internalEndNode);
            void processBarriers();

            std::shared_ptr<geos::geom::MultiLineString> splitLine(const geos::geom::Coordinate& coord,
                                                                   const geos::geom::Geometry& lineGeom);

            std::unique_ptr<geos::geom::MultiLineString> cutLine(const geos::geom::Coordinate& startCoord,
                                                            const geos::geom::Geometry& lineGeom,
                                                            const double cutOff,
                                                            double& cost //out
                                                            );
            double getRelativeValueFromGeomLength(const double attrValue, const geos::geom::MultiLineString& totalGeom,
                                                    const geos::geom::LineString& segmentGeom);
            std::vector<geos::geom::Geometry*> processRouteParts(std::vector<netxpert::data::InternalArc>& routeNodeArcRep);

            netxpert::cnfg::Config NETXPERT_CNFG;
            //TODO
            unsigned int getCurrentNodeCount();
            //TODO
            unsigned int getCurrentArcCount();

            /**
            * For results of a subset of original arcs only
            */
            void saveResults(const std::string& arcIDs, const std::string& resultTableName);


            /**
            * For GEOMETRTY_HANDLING::StraightLines and GEOMETRTY_HANDLING::NoGeometry
            */
            void saveResults(const std::string orig, const std::string dest, const double cost, const double capacity,
                             const double flow, const std::string& resultTableName, netxpert::io::DBWriter& writer);
            /**
            * For GEOMETRTY_HANDLING::StraightLines and GEOMETRTY_HANDLING::NoGeometry
            * and with once prepared statement
            */
            void saveResults(const std::string orig, const std::string dest, const double cost, const double capacity,
                             const double flow, const std::string& resultTableName, netxpert::io::DBWriter& writer,
                             SQLite::Statement& qry);
            /**
            * For results of original arcs and new route parts
            */
            void saveResults(const std::string orig, const std::string dest, const double cost, const double capacity,
                             const double flow, const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                             const std::string& resultTableName, netxpert::io::DBWriter& writer,
                             SQLite::Statement& qry //only used when saved not to the netxpert db
                                        );
            //MCF, TP
            void saveMCFResultsMem(const std::string orig, const std::string dest, const double cost, const double capacity,
                                const double flow, const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                SQLite::Statement& qry  );
            //SPT, ODM
            void saveSPTResultsMem(const std::string orig, const std::string dest, const double cost,
                                const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                SQLite::Statement& qry  );
            //Isolines
            void saveIsoResultsMem(const std::string orig, const double cost,
                                const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                SQLite::Statement& qry,
                                const std::unordered_map<netxpert::data::ExtNodeID, std::vector<double> >& cutOffs);
            //MCF
            double calcTotalDemand ();
            double calcTotalSupply ();
            void transformExtraDemand();
            void transformExtraSupply();
            double getSupplyDemandDifference();
            void processSupplyOrDemand();

            std::string arcIDColName;
            std::string fromColName;
            std::string toColName;
            std::string costColName;
            std::string capColName;
            std::string nodeIDColName;
            std::string supplyColName;
            std::string newSegmentsTempTblName;
            std::string onewayColName;

            netxpert::data::InputArcs arcsTbl;
            netxpert::data::InputNodes nodesTbl;
            netxpert::data::NodeSupplies nodeSupplies;

            //internal network data
            //TODO BiMap
            std::unordered_map<netxpert::data::ExtNodeID,netxpert::data::IntNodeID> internalDistinctNodeIDs;
            std::unordered_map<netxpert::data::IntNodeID,netxpert::data::ExtNodeID> swappedInternalDistinctNodeIDs;
            netxpert::data::AddedPoints addedStartPoints;
            netxpert::data::AddedPoints addedEndPoints;
            std::unordered_set<std::string> eliminatedArcs;
            //TODO: vector?
            std::list<netxpert::data::ExternalArc> arcLoops;
            //TODO: check internal arcs for duplicate arcs with different costs
            netxpert::data::Arcs internalArcData;


            //Network changes
            netxpert::data::Arcs oldArcs;
            netxpert::data::SwappedOldArcs swappedOldArcs;
            netxpert::data::NewArcs newArcs;
    };
}
#endif // NETWORK_H
