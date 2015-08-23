#ifndef NETWORK_H
#define NETWORK_H

#include "utils.h"
#include "dbhelper.h"
#include "slitewriter.h"
#include "fgdbwriter.h"
#include <algorithm>
#include "geos/geom/CoordinateSequenceFactory.h"
#include "geos/geom/GeometryFactory.h"
#include "geos/opDistance.h"
#include "geos/index/strtree/STRtree.h"
#include "geos/linearref/LengthIndexedLine.h"
#include "geos/geom/LineString.h"
#include "geos/linearref/ExtractLineByLocation.h"
#include "geos/linearref/LengthIndexOfPoint.h"
#include "geos/linearref/LocationIndexOfLine.h"
#include "geos/io/WKTReader.h"
#include "geos/opLinemerge.h"
#include <cmath>

using namespace std;
using namespace geos::geom;
using namespace geos::index::strtree;
using namespace geos::operation::distance;
using namespace geos::linearref;
using namespace geos::operation::linemerge;

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
            Network(const InputArcs& arcsTbl, const ColumnMap& _map, const Config& cnfg);
            /**
            * Handles both arcs and nodes from the constructor call which can carry positive supply or negative demand values.
            **/
            Network(const InputArcs& arcsTbl, const InputNodes& nodesTbl, const ColumnMap& _map, const Config& cnfg);
            /**
            * Minimal constructor for the network.
            * TESTME
            **/
            Network(Arcs arcData, unordered_map<ExtArcID,IntNodeID> distinctNodeIDs,
                       NodeSupplies _nodeSupplies);

            unsigned int AddStartNode(const NewNode& newNode, int treshold, SQLite::Statement& closestArcQry, bool withCapacity);
            unsigned int AddEndNode(const NewNode& newNode, int treshold, SQLite::Statement& closestArcQry, bool withCapacity);

            vector< pair<unsigned int, string> > LoadStartNodes(const vector<NewNode>& newNodes, int treshold,
                                                            string arcsTableName, string geomColumnName,
                                                                ColumnMap& cmap, bool withCapacity);
            vector< pair<unsigned int, string> > LoadEndNodes(const vector<NewNode>& newNodes, int treshold,
                                                            string arcsTableName, string geomColumnName,
                                                                ColumnMap& cmap, bool withCapacity);

            void ProcessResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                            const string& arcIDs,
                                            const string& resultTableName);

            void ProcessResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                           const string& arcIDs, vector<InternalArc>& routeNodeArcRep,
                                           const string& resultTableName,
                                           DBWriter& writer);

            void ProcessResultArcs(string orig, string dest, double cost, double capacity, double flow,
                                           const string& arcIDs, vector<InternalArc>& routeNodeArcRep,
                                           const string& resultTableName,
                                           DBWriter& writer, SQLite::Statement& qry);

            void ConvertInputNetwork(bool autoClean);

            //Network Helpers
            SplittedArc GetSplittedClosestNewArcToPoint(Coordinate coord, int treshold);

            SplittedArc GetSplittedClosestOldArcToPoint(Coordinate coord, int treshold,
                                                        const pair<ExternalArc,ArcData>& arcData, const Geometry& arc);

            bool IsPointOnArc(Coordinate coords, const Geometry& arc);

            //Helpers for looking up original data
            vector<string> GetOriginalArcIDs(const vector<InternalArc>& ftNodes, bool isDirected) const;
            vector<ArcData> GetOriginalArcData(const vector<InternalArc>& ftNodes, bool isDirected) const;

            //TODO
            void GetOriginalArcDataAndFlow(const list<ArcDataAndFlow>& origArcDataAndFlow,
                                            const list<InternalArc>& startEndNodes,
                                            bool isDirected);

            string GetOriginalNodeID(unsigned int internalNodeID);
            unsigned int GetInternalNodeID(string externalNodeID);
            string GetOriginalStartOrEndNodeID(unsigned int internalNodeID);
            Coordinate GetStartOrEndNodeGeometry(unsigned int internalNodeID);
            Coordinate GetStartOrEndNodeGeometry(std::string externalNodeID);
            double GetPositionOfPointAlongLine(Coordinate coord, const Geometry& arc);
            StartOrEndLocationOfLine GetLocationOfPointOnLine(Coordinate coord, const Geometry& arc);

            unsigned int GetMaxNodeCount();
            unsigned int GetMaxArcCount();
            unsigned int GetCurrentNodeCount();
            unsigned int GetCurrentArcCount();
            Arcs& GetInternalArcData();
            NodeSupplies GetNodeSupplies();
            Arcs& GetOldArcs();
            NewArcs& GetNewArcs();

            void Reset();

            //MCF
            MinCostFlowInstanceType GetMinCostFlowInstanceType();
            void TransformUnbalancedMCF(MinCostFlowInstanceType mcfInstanceType);

            virtual ~Network();

        private:
            bool isEqual (string& a, string& b) { return a == b; }
            vector<InternalArc> insertNewStartNode(bool isDirected, SplittedArc& splittedLine, string extArcID, string extNodeID,
                                            const Coordinate& startPoint);
            vector<InternalArc> insertNewEndNode(bool isDirected, SplittedArc& splittedLine, string extArcID, string extNodeID,
                                            const Coordinate& endPoint);
            double getRelativeValueFromGeomLength(const double attrValue, const MultiLineString& totalGeom,
                                                    const LineString& segmentGeom);
            void renameNodes();
            void readNetworkFromTable(bool autoClean, bool oneWay);
            void processArc(InputArc arc, unsigned int internalStartNode,
                                    unsigned int internalEndNode);
            //TODO
            void processBarriers();

            shared_ptr<MultiLineString> splitLine(Coordinate coord,
                                        const Geometry& lineGeom);

            vector<Geometry*> processRouteParts(vector<InternalArc>& routeNodeArcRep);

            Config NETXPERT_CNFG;
            //TODO
            unsigned int getCurrentNodeCount();
            //TODO
            unsigned int getCurrentArcCount();

            /**
            * For results of original arcs only
            */
            void saveResults(string orig, string dest, double cost, double capacity, double flow,
                                            const string& arcIDs, const string& resultTableName);
            /**
            * For GEOMETRTY_HANDLING::StraightLines and GEOMETRTY_HANDLING::NoGeometry
            */
            void saveResults(string orig, string dest, double cost, double capacity, double flow,
                                        const string& resultTableName, DBWriter& writer);
            /**
            * For GEOMETRTY_HANDLING::StraightLines and GEOMETRTY_HANDLING::NoGeometry
            * and with once prepared statement
            */
            void saveResults(string orig, string dest, double cost, double capacity, double flow,
                                        const string& resultTableName, DBWriter& writer, SQLite::Statement& qry);
            /**
            * For results of original arcs and new route parts
            */
            void saveResults(string orig, string dest, double cost, double capacity, double flow,
                                        const string& arcIDs, vector<Geometry*> routeParts,
                                        const string& resultTableName, DBWriter& writer);


            //MCF
            double calcTotalDemand ();
            double calcTotalSupply ();
            void transformExtraDemand();
            void transformExtraSupply();
            double getSupplyDemandDifference();
            void processSupplyOrDemand();

            string arcIDColName;
            string fromColName;
            string toColName;
            string costColName;
            string capColName;
            string nodeIDColName;
            string supplyColName;
            string newSegmentsTempTblName;
            string onewayColName;

            InputArcs arcsTbl;
            InputNodes nodesTbl;
            NodeSupplies nodeSupplies;

            //internal network data
            //TODO BiMap
            unordered_map<ExtNodeID,IntNodeID> internalDistinctNodeIDs;
            unordered_map<IntNodeID,ExtNodeID> swappedInternalDistinctNodeIDs;
            AddedPoints addedStartPoints;
            AddedPoints addedEndPoints;
            unordered_set<string> eliminatedArcs;
            list<ExternalArc> arcLoops;
            Arcs internalArcData;

            //Network changes
            Arcs oldArcs;
            SwappedOldArcs swappedOldArcs;
            NewArcs newArcs;
    };
}
#endif // NETWORK_H
