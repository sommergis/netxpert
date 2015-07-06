#ifndef NETWORK_H
#define NETWORK_H

#include "dbhelper.h"
#include "slitewriter.h"
#include <algorithm>
#include <geos/geom/CoordinateSequenceFactory.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/opDistance.h>
#include <geos/index/strtree/STRtree.h>
#include <geos/linearref/LengthIndexedLine.h>
#include <geos/geom/LineString.h>
#include <geos/linearref/ExtractLineByLocation.h>
#include <geos/linearref/LengthIndexOfPoint.h>
#include <geos/linearref/LocationIndexOfLine.h>
#include <geos/io/WKTReader.h>
#include <cmath>

using namespace std;
using namespace geos::geom;
using namespace geos::index::strtree;
using namespace geos::operation::distance;
using namespace geos::linearref;

namespace netxpert
{
    class Network
    {
        public:
            Network(Arcs arcData, unordered_map<ExtArcID,IntNodeID> distinctNodeIDs,
                       NodeSupplies _nodeSupplies);

            //Default constructor
            Network(const InputArcs& arcsTbl, const ColumnMap& _map, const Config& cnfg);
            Network(const InputArcs& arcsTbl, const InputNodes& nodesTbl, const ColumnMap& _map, const Config& cnfg);

            unsigned int AddStartNode(const NewNode& newNode, int treshold, SQLite::Statement& closestArcQry, bool withCapacity);
            unsigned int AddEndNode(const NewNode& newNode, int treshold, SQLite::Statement& closestArcQry, bool withCapacity);

            vector< pair<unsigned int, string> > LoadStartNodes(const vector<NewNode>& newNodes, int treshold,
                                                            string arcsTableName, string geomColumnName,
                                                                ColumnMap& cmap, bool withCapacity);
            vector< pair<unsigned int, string> > LoadEndNodes(const vector<NewNode>& newNodes, int treshold,
                                                            string arcsTableName, string geomColumnName,
                                                                ColumnMap& cmap, bool withCapacity);

            //for subset of arcs
            void BuildTotalRouteGeometry(const string& arcIDs,
                                            const string& resultTableName);
            void BuildTotalRouteGeometry(string& arcIDs, vector<InternalArc>& routeNodeArcRep,
                                         unsigned int orig, unsigned int dest, const string& resultTableName,
                                          DBWriter& writer);

            void ConvertInputNetwork(bool autoClean);

            //Network Helpers
            SplittedArc GetSplittedClosestNewArcToPoint(Coordinate coord, int treshold);

            SplittedArc GetSplittedClosestOldArcToPoint(Coordinate coord, int treshold,
                                                        const pair<ExternalArc,ArcData>& arcData, const Geometry& arc);

            bool IsPointOnArc(Coordinate coords, const Geometry& arc);

            //Helpers for looking up original data
            vector<string> GetOriginalArcIDs(const vector<InternalArc>& ftNodes, bool isDirected) const;

            //TODO
            void GetOriginalArcData(const list<ArcData>& origArcData,
                                    const list<InternalArc>& startEndNodes,
                                    bool isDirected);
            void GetOriginalArcDataAndFlow(const list<ArcDataAndFlow>& origArcDataAndFlow,
                                            const list<InternalArc>& startEndNodes,
                                            bool isDirected);

            string GetOriginalNodeID(unsigned int internalNodeID);
            unsigned int GetInternalNodeID(string externalNodeID);
            string GetOriginalStartOrEndNodeID(unsigned int internalNodeID);
            void GetStartOrEndNodeGeometry(Coordinate& coord, unsigned int internalNodeID);
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

            Config netXpertConfig;
            //TODO
            unsigned int getCurrentNodeCount();
            //TODO
            unsigned int getCurrentArcCount();

            void buildTotalRouteGeometry(const string& arcIDs, const string& resultTableName);
            void buildTotalRouteGeometry(const string& arcIDs, vector<Geometry*> tmpRes,
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
