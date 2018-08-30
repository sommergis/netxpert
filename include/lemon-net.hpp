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

#ifndef LEMONNET_H
#define LEMONNET_H

#include <cmath>
#include <unordered_map>
#include <vector>
#include <set>
#include <string>
#include <memory>
#include <algorithm>

#include <iomanip> // setprecision for string output on decimal values

#include "lemon/time_measure.h"

#if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)
    #include "CHInterface.h"
    #include "CH/DefaultPriority.h"
#endif // NETX_ENABLE_CONTRACTION_HIERARCHIES

#include "geos/geom/CoordinateSequenceFactory.h"
#include "geos/geom/GeometryFactory.h"

#include "geos/linearref/LengthIndexedLine.h"
#include "geos/linearref/ExtractLineByLocation.h"
#include "geos/linearref/LocationIndexedLine.h"
#include "geos/linearref/LengthLocationMap.h"
#include "geos/linearref/LinearLocation.h"

#include "geos/io/WKTReader.h"
#include "geos/opLinemerge.h"
#include "geos/opDistance.h"

#include "data.hpp"
#include "dbhelper.hpp"
#include "fgdbwriter.hpp"
#include "slitewriter.hpp"

namespace netxpert {

    namespace data {
      /**
      * \brief Combined structure of graph attribtues as LEMON Reference Map.
      * \warning Not used!
      **/
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
          /// gets the capacity for given arc
         const capacity_t getCap(const Key& e) const {
          return this->data[e].capacity;
         }

       };
    }

    namespace data {
    /**\brief The internal Network representation.
    **/
    class InternalNet
    {
      public:
        ///\brief Constructor with default values
        ///\param arcsTbl: represents arcs read from a database (mandatory).
        ///\param _map: column names
        ///\param cnfg: Config for netxpert
        ///\param nodesTbl: represents nodes read from a database.
        /// These nodes will be used for split up arcs if necessary (optional).
        ///\param autoClean: clean the network on reading
        ///\param extIntNodeMap: represents a mapping between external node IDs and internal node IDs.
        InternalNet(const netxpert::data::InputArcs& arcsTbl,
                    const netxpert::data::ColumnMap& _map = netxpert::data::ColumnMap(),
                    const netxpert::cnfg::Config& cnfg = netxpert::cnfg::Config(),
                    const netxpert::data::InputNodes& nodesTbl = netxpert::data::InputNodes(),
                    const bool autoClean = true,
                    const std::map<std::string, netxpert::data::IntNodeID>& extIntNodeMap = std::map<std::string, netxpert::data::IntNodeID>() );

        /// Empty Destructor.
        ~InternalNet() {}

        //-->Region Getters

        /* Access Arcs of internal network */
        ///\brief Gets arc from internal ID
        const netxpert::data::arc_t
         GetArcFromID(const netxpert::data::intarcid_t arcID);

        ///\brief Gets arc from original ID
        const netxpert::data::arc_t
         GetArcFromOrigID(const netxpert::data::extarcid_t arcID);

        ///\brief Gets internal arc ID by passing an internal arc
        const netxpert::data::intarcid_t
         GetArcID(const netxpert::data::arc_t& arc);

        ///\brief Gets the relevant arc data object by passing an internal arc
        const netxpert::data::ArcData
        //netxpert::data::ArcData2
         GetArcData(const netxpert::data::arc_t& arc);

        ///\brief Assign arc data to given internal arc
        void
         SetArcData(const netxpert::data::arc_t& arc,
                    const netxpert::data::ArcData& arcData);

        ///\brief Gets arc cost by passing internal arc
        const netxpert::data::cost_t
         GetArcCost(const netxpert::data::arc_t& arc);

        ///\brief Gets arc capacity by passing internal arc
        const netxpert::data::capacity_t
         GetArcCapacity(const netxpert::data::arc_t& arc);

        ///\brief Gets a set of unordered original arc IDs by passing a vector of internal arcs
        const std::unordered_set<std::string>
         GetOrigArcIDs(const std::vector<netxpert::data::arc_t>& path);

        ///\brief Gets internal arc by passing a source and target node
        const netxpert::data::arc_t
         GetArcFromNodes(const netxpert::data::node_t& source,
                         const netxpert::data::node_t& target);

        /* Access Nodes of internal network */
        ///\brief Gets internal node by passing an original node ID
        const netxpert::data::node_t
         GetNodeFromOrigID(const std::string nodeID);

        ///\brief Gets original node ID by passing internal node
        const std::string
         GetOrigNodeID(const netxpert::data::node_t& node);

        ///\brief Gets internal node ID cost by passing internal node
        const uint32_t
         GetNodeID(const netxpert::data::node_t& node);

        ///\brief Gets internal node by passing internal node ID
        const netxpert::data::node_t
         GetNodeFromID(const uint32_t nodeID);

        ///\brief Gets node supply by passing internal node
        const netxpert::data::supply_t
         GetNodeSupply(const netxpert::data::node_t& node);

        ///\brief Gets nodes iterator on internal graph
        const netxpert::data::graph_t::NodeIt
         GetNodesIter();

        ///\brief Gets arcs iterator on internal graph
        const netxpert::data::graph_t::ArcIt
         GetArcsIter();

        ///\brief Gets source node by passing an internal node
        const netxpert::data::node_t
         GetSourceNode(const netxpert::data::arc_t& arc);

        ///\brief Gets target node by passing an internal node
        const netxpert::data::node_t
         GetTargetNode(const netxpert::data::arc_t& arc);

        ///\brief Gets coordinate of node by passing an original node ID
        const geos::geom::Coordinate
         GetStartOrEndNodeGeometry(const netxpert::data::ExtNodeID node);

        ///\brief Gets count of all nodes in the internal graph
        const uint32_t
         GetNodeCount();

        ///\brief Gets count of all arcs in the internal graph
        const uint32_t
         GetArcCount();

        ///\brief Gets a pointer to the internal graph
        netxpert::data::graph_t*
         GetGraph() {
            return this->g.get();
         };

        ///\brief Get a pointer to the internal arc cost map
        netxpert::data::graph_t::ArcMap<netxpert::data::cost_t>*
         GetCostMap() {
            return this->costMap.get();
//                netxpert::data::graph_t::ArcMap<netxpert::data::cost_t> cm(*this->g);
//                this->arcDataMap->fillCostMap(cm);
         };

        ///\brief Gets a pointer to the internal arc capacity map
        netxpert::data::graph_t::ArcMap<netxpert::data::capacity_t>*
         GetCapMap() {
            return this->capMap.get();
         };

        ///\brief Gets a pointer to the internal arc filter map
        netxpert::data::graph_t::ArcMap<bool>*
         GetArcFilterMap() {
            return this->arcFilterMap.get();
         };

        ///\brief Gets a pointer to the internal node supply map
        netxpert::data::graph_t::NodeMap<netxpert::data::supply_t>*
         GetSupplyMap() {
            return this->nodeSupplyMap.get();
        };

        ///\brief Gets a pointer to the internal node map (internal nodes: original node ID)
        netxpert::data::graph_t::NodeMap<std::string>*
         GetNodeMap() {
            return this->nodeMap.get();
        };

        ///\brief Gets a map of original node IDs to internal nodes
        const std::unordered_map<std::string, netxpert::data::node_t>
         GetNodeIDMap() {
            return this->nodeIDMap;
        };

        ///\brief Registers the original node ID to the internal node
        void
         RegisterNodeID(const std::string& nodeID, const netxpert::data::node_t& node);

        //--|Region Getters

        //-->Region Add Points

        ///\brief Adds a new node in the network (internal -> private??)
        ///
        ///- fetch the nearest arc and the closest point on the arc to the new node (via Spatialite): arcID, arc geometry and closest point geometry
        ///- has the arc already been split by a new node?
        ///- if yes: fetch the geometry from the internal already split arcs
        ///- check for location of node on line: isPointOnLine, position of closestPoint (in memory with geos)
        ///- split arc geometrically and insert new node in internal graph
        ///\return internal node ID
        const uint32_t
         AddNode(const netxpert::data::NewNode& newNode,
                                   const int threshold,
                                   SQLite::Statement& closestArcQry,
                                   const bool withCapacity,
                                   const netxpert::data::AddedNodeType startOrEnd);

        ///\brief Method for adding start nodes
        ///\return internal node ID
        const uint32_t
         AddStartNode(std::string extNodeID,
                                  double x, double y, netxpert::data::supply_t supply,
                                  int threshold,
                                  const netxpert::data::ColumnMap& cmap,
                                  bool withCapacity);

        ///\brief Method for adding end nodes
        ///\return internal node ID
        const uint32_t
         AddEndNode(std::string extNodeID,
                                double x, double y, netxpert::data::supply_t supply,
                                int threshold,
                                const netxpert::data::ColumnMap& cmap,
                                bool withCapacity);

        ///\brief Simple Method for adding multiple start nodes
        ///\return A vector of pairs with internal node ID and the original node ID
        std::vector< std::pair<uint32_t, std::string> >
         LoadStartNodes(std::vector<netxpert::data::NewNode> newNodes, const int threshold,
                        const std::string arcsTableName, const std::string geomColumnName,
                        const netxpert::data::ColumnMap& cmap, const bool withCapacity);

        ///\brief Simple Method for adding multiple end nodes
        ///\return A vector of pairs with internal node ID and the original node ID
        std::vector< std::pair<uint32_t, std::string> >
         LoadEndNodes(std::vector<netxpert::data::NewNode> newNodes, const int threshold,
                      const std::string arcsTableName, const std::string geomColumnName,
                      const netxpert::data::ColumnMap& cmap, const bool withCapacity);
        void
         Reset();

        //--|Region Add Points

        //-->Region Save Results
        /**\brief Main method for processing and saving result arcs (preloading geometry into memory).
            Writes results to a database (SpatiaLite or ESRI File Geodatabase).
            Solver: SPT, ODM */
        void
         ProcessSPTResultArcsMem(const std::string& orig, const std::string& dest, const netxpert::data::cost_t cost,
                                     const std::string& arcIDs, const std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                     const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                     SQLite::Statement& qry //can be null in case of ESRI FileGDB
                                    );
        /**\brief Main method for processing and saving result arcs (preloading geometry into memory).
            Writes results to a stringstream (JSON).
            Solver: SPT, ODM */
        void
         ProcessSPTResultArcsMemS(const std::string& orig, const std::string& dest, const netxpert::data::cost_t cost,
                                       const std::string& arcIDs, const std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                       std::ostringstream& output);
        /**\brief Method for processing and saving a subset of original arcs as results
            Solver: MST */
        void
         ProcessResultArcs(const std::string& arcIDs, const std::string& resultTableName);

        /**\brief Main method for processing and saving result arcs (preloading geometry into memory)
            Solver: Isolines */
        void
         ProcessIsoResultArcsMem(const std::string& orig, const netxpert::data::cost_t cost,
                                 const std::string& arcIDs, const std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                 const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                 SQLite::Statement& qry,
                                 const std::unordered_map<netxpert::data::ExtNodeID, std::vector<double> > cutOffs);

        /**\brief Main method for processing and saving result arcs (preloading geometry into memory)
            Solver: MCF, TPs */
        void ProcessMCFResultArcsMem(const std::string& orig, const std::string& dest, const netxpert::data::cost_t cost,
                                     const netxpert::data::capacity_t capacity, const netxpert::data::flow_t flow,
                                     const std::string& arcIDs, std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                     const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                     SQLite::Statement& qry //can be null in case of ESRI FileGDB
                                    );
        /** Main method for processing and saving result arcs (preloading geometry into memory)
            Writes results to a stringstream (JSON).
            Solver: MCF, TPs */
        void ProcessMCFResultArcsMemS(const std::string& orig, const std::string& dest, const netxpert::data::cost_t cost,
                                      const netxpert::data::capacity_t capacity, const netxpert::data::flow_t flow,
                                      const std::string& arcIDs, std::vector<netxpert::data::arc_t>& routeNodeArcRep,
                                      std::ostringstream& output);
        //--|Region Save Results

        ///\brief prints the graph in simple form.
        void
         PrintGraph();

        ///\brief exports the graph into DIMACS format.
        void
         ExportToDIMACS(const std::string& path);

//      -->Region Contraction Hierarchies

        #if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)
        ///\brief Computes contraction hierarchies to speed up shortest path queries
        void
         ComputeContraction(float contractionPercent);

        ///\brief Exports contraction hierarchies
        void
         ExportContractedNetwork(const std::string graphName);

        ///\brief Imports contraction hierarchies
        void
         ImportContractedNetwork(const std::string graphName);

        ///\brief Returns if network has contraction hierarchies
        bool
         GetHasContractionHierarchies() {
          return this->hasContractionHierarchies;
         };

        ///\brief Gets contraction hierarchy manager
        CHInterface<DefaultPriority>*
         GetCHManager() {
          return this->chManager.get();
        };

        ///\brief Gets contraction hierarchy cost map
        netxpert::data::graph_ch_t::ArcMap<netxpert::data::cost_t>*
         GetCostMap_CH() {
          return this->chCostMap.get();
        };

        ///\brief Gets contraction hierarchy node map
        netxpert::data::graph_t::NodeMap<netxpert::data::graph_ch_t::Node>*
         GetNodeMap_CH() {
          return this->chNodeRefMap.get();
        }

        ///\brief Gets contraction hierarchy cross ref node map
        netxpert::data::graph_ch_t::NodeMap<netxpert::data::graph_t::Node>*
         GetNodeCrossRefMap_CH() {
          return this->chNodeCrossRefMap.get();
        }
        //|-->Region Contration Hierarchies
        #endif

        //-->Region MinCostFlow functions

        ///\brief Gets contraction hierarchy node map
        netxpert::data::MinCostFlowInstanceType
         GetMinCostFlowInstanceType();

         ///\brief Transforms an unbalanced MCFP through dummy nodes.
         /// Goal is a balanced distribution of supply and demand.
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
       getSplittedClosestOrigArcToPoint(const geos::geom::Coordinate coord, const int threshold,
                                        const std::pair<netxpert::data::ExternalArc,netxpert::data::ArcData>& arcData,
                                        const geos::geom::Geometry& arc);

      const netxpert::data::IntNetSplittedArc<netxpert::data::arc_t>
       getSplittedClosestNewArcToPoint(const geos::geom::Coordinate& coord,
                                        const int threshold);

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

      std::unique_ptr<geos::geom::LineString>
       getStraightLine(const std::string& orig, const std::string& dest);

      std::string
       convertRouteToCoordList(std::unique_ptr<geos::geom::MultiLineString>& route);

      void
       saveSPTResultsMem(const std::string orig, const std::string dest, const netxpert::data::cost_t cost,
                                      const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                      const std::string& resultTableName, netxpert::io::DBWriter& writer,
                                      SQLite::Statement& qry );
      void
       saveSPTResultsMemS(const std::string orig, const std::string dest, const netxpert::data::cost_t cost,
                                const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                std::ostringstream& outfile);

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

      void saveMCFResultsMemS(const std::string orig, const std::string dest, const netxpert::data::cost_t cost,
                                 const netxpert::data::capacity_t capacity, const netxpert::data::flow_t flow,
                                 const std::string& arcIDs, std::vector<geos::geom::Geometry*> routeParts,
                                 std::ostringstream& outStream);
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

      //-->Region Contraction Hierarchies
      void
       fillNodeIDMap();

      void
       fillNodeMap();
      //|-->Region Contraction Hierarchies


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

      //Contraction Hierarchies
      #if (defined NETX_ENABLE_CONTRACTION_HIERARCHIES)

      bool hasContractionHierarchies = false;
      std::unique_ptr<netxpert::data::graph_ch_t> chg;
      std::unique_ptr<netxpert::data::graph_ch_t::ArcMap<netxpert::data::cost_t>> chCostMap;
      std::unique_ptr<netxpert::data::graph_t::NodeMap<netxpert::data::graph_ch_t::Node>> chNodeRefMap;
      //internal CH node ID, external node ID from input graph
      std::map<std::string, int> chNodeIDRefMap;
      std::unique_ptr<CHInterface<DefaultPriority>> chManager;
      std::unique_ptr<netxpert::data::graph_ch_t::NodeMap<netxpert::data::graph_t::Node>> chNodeCrossRefMap;

      #endif

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
      /// stores the information whether to use arcs or not in the solver
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
    }//namespace data
}

#endif // LEMONNET_H
