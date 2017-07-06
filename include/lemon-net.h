#ifndef LEMONNET_H
#define LEMONNET_H

#include <cmath>
#include <unordered_map>
#include <vector>
#include <set>
#include <string>
#include <memory>
#include <algorithm>

#include <lemon/smart_graph.h>
#include <lemon/maps.h>

#include "geos/geom/CoordinateSequenceFactory.h"
#include "geos/geom/GeometryFactory.h"

#include "geos/linearref/LengthIndexedLine.h"
#include "geos/linearref/ExtractLineByLocation.h"
#include "geos/linearref/LengthIndexOfPoint.h"
#include "geos/linearref/LocationIndexOfLine.h"
#include "geos/linearref/LocationIndexedLine.h"
#include "geos/linearref/LengthLocationMap.h"
#include "geos/io/WKTReader.h"
#include "geos/opLinemerge.h"
#include "geos/opDistance.h"

#include "utils.h"
#include "data.h"
#include "dbhelper.h"
#include "fgdbwriter.h"
#include "slitewriter.h"


namespace netxpert {

    namespace data {

        template<typename GR, typename K, typename T, typename R = T&, typename CR = const T&>
        class ArcDataMap : public lemon::concepts::ReferenceMap<K, T, R, CR>
        {
         typedef lemon::SmartDigraph graph_t;
         typedef double cost_t;
         typedef double capacity_t;
         typedef K Key;
         typedef T Value;
         typedef R Reference;
         typedef CR ConstReference;

         private:
           const graph_t &g;
           graph_t::ArcMap<Value> data;

         public:
            //Value und Key sind public typedefs aus der abgeleiteten Klasse
           //typedef GR graph_t;
           //typedef CO cost_t;
           //typedef CA capacity_t;

           ///Constructor
           ///initialize map "_data" on construction
           ArcDataMap(const GR &_g) : g(_g), data (_g) {};

           Reference operator[](const Key& e) {
            return this->data[e];
           }

           ConstReference operator[](const Key& e) const {
            return this->data[e];
           }
           /* does not work
           double operator[](const Key& e) const {
            return this->m[e].cost;
           }*/

           void set(const Key& k, const Value& v) {
            this->data[k] = v;
           }

           /// fills the cost map that is passed in by reference
           void fillCostMap(graph_t::ArcMap<cost_t>& _costMap) {
            for (graph_t::ArcIt it(this->g); it != lemon::INVALID; ++it) {
              Value a = this->operator[](it);
              _costMap[it] = a.cost;
            }
           }
           /// fills the capacity map that is passed in by reference
           void fillCapMap(graph_t::ArcMap<capacity_t>& _capMap) {
            for (graph_t::ArcIt it(this->g); it != lemon::INVALID; ++it) {
              Value a = this->operator[](it);
              _capMap[it] = a.capacity;
            }
           }
           /// gets the cost for given arc
           const cost_t getCost(const Key& e) const {
            return this->data[e].cost;
           }
            /// gets the cost for given arc
           const capacity_t getCap(const Key& e) const {
            return this->data[e].capacity;
           }

         };
    }

    ///\brief The internal Network represenation.
    ///
    ///
    ///
    class InternalNet
    {

        public:
            /// Constructor.
            ///\param The first argument \c arcsTbl represents arcs read from a database.
            /// This is mandatory.
            /*///\param The second argument \c nodesTbl represents nodes read from a database.
            /// These nodes will be used for split up arcs if necessary. \c nodesTbl is optional.*/
            InternalNet(const netxpert::data::InputArcs& arcsTbl,
                        const netxpert::data::ColumnMap& _map = netxpert::data::ColumnMap(),
                        const netxpert::cnfg::Config& cnfg = netxpert::cnfg::Config(),
                        const netxpert::data::InputNodes& nodesTbl = netxpert::data::InputNodes(),
                        const bool autoClean = true,
                        const std::map<std::string, netxpert::data::IntNodeID>& extIntNodeMap = std::map<std::string, netxpert::data::IntNodeID>() );

            /// Deconstructor.
            ~InternalNet() {}

            //-->Region Getters

            /* Access Arcs of internal network */
            const netxpert::data::arc_t
             GetArcFromID(const netxpert::data::intarcid_t arcID);

            const netxpert::data::arc_t
             GetArcFromOrigID(const netxpert::data::extarcid_t arcID);

            const netxpert::data::intarcid_t
             GetArcID(const netxpert::data::arc_t& arc);

            const netxpert::data::ArcData
            //netxpert::data::ArcData2
             GetArcData(const netxpert::data::arc_t& arc);

            void
             SetArcData(const netxpert::data::arc_t& arc,
                        const netxpert::data::ArcData& arcData);

            const netxpert::data::cost_t
             GetArcCost(const netxpert::data::arc_t& arc);

            const netxpert::data::capacity_t
             GetArcCapacity(const netxpert::data::arc_t& arc);

            const std::unordered_set<std::string>
             GetOrigArcIDs(const std::vector<netxpert::data::arc_t>& path);

            const netxpert::data::arc_t
             GetArcFromNodes(const netxpert::data::node_t& source,
                             const netxpert::data::node_t& target);

            /* Access Nodes of internal network */
            const netxpert::data::node_t
             GetNodeFromOrigID(const std::string nodeID);

            const std::string
             GetOrigNodeID(const netxpert::data::node_t& node);

            const uint32_t
             GetNodeID(const netxpert::data::node_t& node);

            const netxpert::data::node_t
             GetNodeFromID(const uint32_t nodeID);

            const netxpert::data::supply_t
             GetNodeSupply(const netxpert::data::node_t& node);

            const netxpert::data::graph_t::NodeIt
             GetNodesIter();

            const netxpert::data::graph_t::ArcIt
             GetArcsIter();

            const netxpert::data::node_t
             GetSourceNode(const netxpert::data::arc_t& arc);

            const netxpert::data::node_t
             GetTargetNode(const netxpert::data::arc_t& arc);

            const geos::geom::Coordinate
             GetStartOrEndNodeGeometry(const netxpert::data::ExtNodeID node);

            const uint32_t
             GetNodeCount();

            const uint32_t
             GetArcCount();

            netxpert::data::graph_t*
             GetGraph() {
                return this->g.get();
             };

            netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>*
             GetCostMap() {
                return this->costMap.get();
//                netxpert::data::graph_t::ArcMap<netxpert::data::cost_t> cm(*this->g);
//                this->arcDataMap->fillCostMap(cm);
             };

            netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>*
             GetCapMap() {
                return this->capMap.get();
             };

            netxpert::data::graph_t::ArcMap<bool>*
             GetArcFilterMap() {
                return this->arcFilterMap.get();
             };

            netxpert::data::graph_t::NodeMap<netxpert::data::supply_t>*
             GetSupplyMap() {
                return this->nodeSupplyMap.get();
            };

            void
             SetNodeData(const std::string& nodeID, const netxpert::data::node_t& node);

            //--|Region Getters

            //-->Region Add Points

            const uint32_t
             AddNode(const netxpert::data::NewNode& newNode,
                                       const int treshold,
                                       SQLite::Statement& closestArcQry,
                                       const bool withCapacity,
                                       const netxpert::data::AddedNodeType startOrEnd);
            /**
            * Simple Method for adding start nodes
            */
            const uint32_t
             AddStartNode(std::string extNodeID,
                                      double x, double y, netxpert::data::supply_t supply,
                                      int treshold,
                                      const netxpert::data::ColumnMap& cmap,
                                      bool withCapacity);
            /**
            * Simple Method for adding end nodes
            */
            const uint32_t
             AddEndNode(std::string extNodeID,
                                    double x, double y, netxpert::data::supply_t supply,
                                    int treshold,
                                    const netxpert::data::ColumnMap& cmap,
                                    bool withCapacity);

            /**
            * Simple Method for adding multiple start nodes
            */
            std::vector< std::pair<uint32_t, std::string> >
             LoadStartNodes(std::vector<netxpert::data::NewNode> newNodes, const int treshold,
                            const std::string arcsTableName, const std::string geomColumnName,
                            const netxpert::data::ColumnMap& cmap, const bool withCapacity);

            /**
            * Simple Method for adding multiple end nodes
            */
            std::vector< std::pair<uint32_t, std::string> >
             LoadEndNodes(std::vector<netxpert::data::NewNode> newNodes, const int treshold,
                          const std::string arcsTableName, const std::string geomColumnName,
                          const netxpert::data::ColumnMap& cmap, const bool withCapacity);
            void
             Reset();

            //--|Region Add Points

            //-->Region Save Results
            /** Main method for processing and saving result arcs (preloading geometry into memory)
                Solver: SPT, ODM
            */
            void
             ProcessSPTResultArcsMem(const std::string& orig, const std::string& dest, const netxpert::data::cost_t cost,
                                         const std::string& arcIDs, const std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                         const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                         SQLite::Statement& qry //can be null in case of ESRI FileGDB
                                        );
            /** Method for processing and saving a subset of original arcs as results
                Solver: MST
            */
            void
             ProcessResultArcs(const std::string& arcIDs, const std::string& resultTableName);

            /** Main method for processing and saving result arcs (preloading geometry into memory)
                Solver: Isolines
            */
            void
             ProcessIsoResultArcsMem(const std::string& orig, const netxpert::data::cost_t cost,
                                     const std::string& arcIDs, const std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                     const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                     SQLite::Statement& qry,
                                     const std::unordered_map<netxpert::data::ExtNodeID, std::vector<double> > cutOffs);

            /** Main method for processing and saving result arcs (preloading geometry into memory)
                Solver: MCF, TPs
                */
            void ProcessMCFResultArcsMem(const std::string& orig, const std::string& dest, const netxpert::data::cost_t cost,
                                         const netxpert::data::capacity_t capacity, const netxpert::data::flow_t flow,
                                         const std::string& arcIDs, std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                         const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                         SQLite::Statement& qry //can be null in case of ESRI FileGDB
                                        );
            //--|Region Save Results

            void
             PrintGraph();

            void
             ExportToDIMACS(const std::string& path);

            //-->Region MinCostFlow functions

            netxpert::data::MinCostFlowInstanceType
             GetMinCostFlowInstanceType();

            /** Arbeiten mit Dummy Nodes; Ziel ist die Ausgewogene Verteilung von Angebot und Nachfrage */
            void
             TransformUnbalancedMCF(netxpert::data::MinCostFlowInstanceType mcfInstanceType);

            //--|Region MinCostFlow functions

        private:

            //-->Region Network core functions
            void
             readNodes(const netxpert::data::InputArcs& arcsTbl,
                       const netxpert::data::InputNodes& nodesTbl,
                       const std::map<std::string, netxpert::data::IntNodeID>& extIntNodeMap);

            void
             processBarriers();

            void
             readNetwork(const netxpert::data::InputArcs& arcsTbl,
                             const bool autoClean,
                             const bool isDirected);

            std::vector<std::string>
             getDistinctOrigNodes(const netxpert::data::InputArcs& arcsTbl);

            void
             processArc(const netxpert::data::InputArc& arc,
                        const netxpert::data::node_t internalStartNode,
                        const netxpert::data::node_t internalEndNode);

            //--|Region Network core functions


            //-->Region Add Points
            const netxpert::data::node_t
             insertNewNode(bool isDirected, netxpert::data::IntNetSplittedArc<netxpert::data::arc_t>& splittedLine,
                               const std::string& extArcID, const std::string& extNodeID,
                               const geos::geom::Coordinate& startPoint,
                               const netxpert::data::AddedNodeType startOrEnd);

            //--|Region Add Point

            //-->Region Barriers

            void insertNewBarrierNodes(bool isDirected, netxpert::data::IntNetSplittedArc2<netxpert::data::arc_t>& clippedLine);
            //--|Region Barriers

            //-->Region Geo Utils
            const double
             getPositionOfPointAlongLine(const geos::geom::Coordinate& coord,
                                         const geos::geom::Geometry& arc);

            const netxpert::data::StartOrEndLocationOfLine
             getLocationOfPointOnLine(const geos::geom::Coordinate& coord,
                                      const geos::geom::Geometry& arc);
            template<typename T>
            const T
             getRelativeValueFromGeomLength(const T attrValue,
                                            const geos::geom::MultiLineString& totalGeom,
                                            const geos::geom::LineString& segmentGeom);

            const netxpert::data::IntNetSplittedArc<netxpert::data::arc_t>
             getSplittedClosestOrigArcToPoint(const geos::geom::Coordinate coord, const int treshold,
                                              const std::pair<netxpert::data::ExternalArc,netxpert::data::ArcData>& arcData,
                                              const geos::geom::Geometry& arc);

            const netxpert::data::IntNetSplittedArc<netxpert::data::arc_t>
             getSplittedClosestNewArcToPoint(const geos::geom::Coordinate& coord,
                                              const int treshold);

//            std::shared_ptr<geos::geom::MultiLineString>
            std::pair<std::shared_ptr<geos::geom::Geometry>, std::shared_ptr<geos::geom::Geometry>>
             splitLine(const geos::geom::Coordinate& coord,
                       const geos::geom::Geometry& lineGeom);

            std::vector<std::shared_ptr<geos::geom::Geometry>>
             clipLine(const geos::geom::Geometry& polyGeom,
                      const geos::geom::Geometry& lineGeom);

            std::unique_ptr<geos::geom::MultiLineString>
             cutLine(const geos::geom::Coordinate& startCoord,
                     const geos::geom::Geometry& lineGeom,
                     const netxpert::data::cost_t cutOff,
                     netxpert::data::cost_t& cost);

            const bool
             isPointOnArc(const geos::geom::Coordinate& coords,
                           const geos::geom::Geometry& arc);

            //--|Region Geo Utils

            //-->Region Save Results

            std::vector<geos::geom::Geometry*>
                addRoutePartGeoms(const std::vector<netxpert::data::arc_t>& routeNodeArcRep);

            void
             saveSPTResultsMem(const std::string orig, const std::string dest, const netxpert::data::cost_t cost,
                                            const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                            const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                            SQLite::Statement& qry );

            //Isolines
            void saveIsoResultsMem(const std::string orig, const netxpert::data::cost_t cost,
                                    const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                    const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                    SQLite::Statement& qry,
                                    const std::unordered_map<netxpert::data::ExtNodeID, std::vector<double> >& cutOffs);

            //MCF
            void saveMCFResultsMem(const std::string orig, const std::string dest, const netxpert::data::cost_t cost,
                                 const netxpert::data::capacity_t capacity, const netxpert::data::flow_t flow,
                                 const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                 const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                 SQLite::Statement& qry);
            //MST
            void
             saveResults(const std::string& arcIDs, const std::string& resultTableName);

            //--|Region Save Results

            //-->Region MinCostFlow functions

            double
             calcTotalSupply ();

            double
             calcTotalDemand ();

            void
             transformExtraDemand();

            void
             transformExtraSupply();

            void
             processSupplyOrDemand();

            double
             getSupplyDemandDifference();

            //--|Region MinCostFlow functions




            //-->Region data members
            std::unique_ptr<netxpert::data::graph_t> g;
            //std::unique_ptr<netxpert::data::graph_t::ArcMap<uint32_t>> extArcIDMap;
            std::unique_ptr<netxpert::data::graph_t::ArcMap<netxpert::data::extarcid_t>> extArcIDMap;
            std::unique_ptr<netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>> costMap;
            std::unique_ptr<netxpert::data::graph_t::ArcMap<netxpert::data::capacity_t>> capMap;
            /*std::unique_ptr<lemon::SourceMap<graph_t> > sourceMap;
            std::unique_ptr<lemon::TargetMap<graph_t> > targetMap;*/
            std::unique_ptr<netxpert::data::graph_t::NodeMap<std::string>> nodeMap;
            std::unique_ptr<netxpert::data::graph_t::NodeMap<netxpert::data::supply_t>> nodeSupplyMap;
            //std::unordered_map<uint32_t, netxpert::data::arc_t> arcIDMap;
            std::unordered_map<std::string, netxpert::data::node_t> nodeIDMap;

            //replacement for other maps
            std::unique_ptr<netxpert::data::ArcDataMap<netxpert::data::graph_t,
                                           netxpert::data::arc_t,
                                           netxpert::data::ArcData> > arcDataMap;

            //TEST
            // Changes

            // indicates if an arc has been split
            //std::unique_ptr<graph_t::ArcMap<bool>> splittedArcsMap

            /// stores all changes that were made to the network through the states of each arc
            std::unique_ptr<netxpert::data::graph_t::ArcMap<netxpert::data::ArcState>> arcChangesMap;
            /// stores data of original arcs that must be tracked for resetting the network
            std::map<netxpert::data::arc_t, netxpert::data::ArcData> origArcsMap;
            //std::unique_ptr<graph_t::ArcMap<netxpert::data::NewArc>> newArcsMap;
            /// stores data of new arcs that have been added through adding a new node
            std::map<netxpert::data::arc_t, netxpert::data::NewArc> newArcsMap;
            /// stores data of new nodes that have been added to the graph
            std::map<netxpert::data::node_t, netxpert::data::NewNode> newNodesMap;
            /// stores the added start nodes that were added to the network
            std::map<netxpert::data::node_t, netxpert::data::AddedPoint> addedStartPoints;
            /// stores the added end nodes that were added to the network
            std::map<netxpert::data::node_t, netxpert::data::AddedPoint> addedEndPoints;
            /// stores the external arc ids that were eliminated in the network
            std::unordered_set<netxpert::data::extarcid_t> eliminatedArcs;
            /// stores the information wether to use arcs or not in the solver
            /// will be computed from the all other maps for arcs change
            std::unique_ptr<netxpert::data::graph_t::ArcMap<bool>> arcFilterMap;

            netxpert::cnfg::Config NETXPERT_CNFG;

            std::string arcIDColName;
            std::string fromColName;
            std::string toColName;
            std::string costColName;
            std::string capColName;
            std::string nodeIDColName;
            std::string supplyColName;
            std::string newSegmentsTempTblName;
            std::string onewayColName;

            //--|Region data members
            //
    };

    //}
}


/*class lemon-net
{
    public:
        lemon-net();
        virtual ~lemon-net() {}

    private:
        netxpert::data::internalArcData1 net1;
        netxpert::data::internalArcData2 net2;
};*/

#endif // LEMONNET_H
