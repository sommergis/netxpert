#ifndef NETWORK_H
#define NETWORK_H

#include "dbhelper.h"
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

using namespace std;
using namespace geos::geom;
using namespace geos::index::strtree;
using namespace geos::operation::distance;
using namespace geos::linearref;

namespace NetXpert
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
            void AddEndNode();
            void BuildTotalRouteGeometry();
            void ConvertInputNetwork(bool autoClean);

            //Network Helpers
            SplittedArc GetSplittedClosestNewArcToPoint(Coordinate coord, int treshold);

            SplittedArc GetSplittedClosestOldArcToPoint(Coordinate coord, int treshold,
                                                        const pair<ExtFTNode,ArcData>& arcData, const Geometry& arc);

            bool IsPointOnArc(Coordinate coords, const Geometry& arc);

            //Helpers for looking up original data
            void GetOriginalArcData(const list<ArcData>& origArcData,
                                    const list<FTNode>& startEndNodes,
                                    bool isDirected);
            void GetOriginalArcDataAndFlow(const list<ArcDataAndFlow>& origArcDataAndFlow,
                                            const list<FTNode>& startEndNodes,
                                            bool isDirected);
            string GetOriginalNodeID(unsigned int internalNodeID);
            unsigned int GetInternalNodeID(string externalNodeID);
            string GetOriginalStartOrEndNodeID(unsigned int internalNodeID);
            void GetStartOrEndNodeGeometry(Coordinate& coord, unsigned int internalNodeID);
            double GetPositionOfPointAlongLine(Coordinate coord, const Geometry& arc);
            StartOrEndLocationOfLine GetLocationOfPointOnLine(Coordinate coord, const Geometry& arc);

            virtual ~Network();

        private:
            bool isEqual (string& a, string& b) { return a == b; }
            vector<FTNode> insertNewStartNode(bool isDirected, SplittedArc& splittedLine, string extNodeID,
                                            const Coordinate& startPoint);
            vector<FTNode> insertNewEndNode(bool isDirected, SplittedArc& splittedLine, string extNodeID,
                                            const Coordinate& endPoint);
            void renameNodes();
            void readNetworkFromTable(bool autoClean, bool oneWay);
            void processArc(InputArc arc, unsigned int internalStartNode,
                                    unsigned int internalEndNode);
            void processBarriers();

            shared_ptr<MultiLineString> splitLine(Coordinate coord,
                                        const Geometry& lineGeom);

            Config netXpertConfig;
            unsigned int currentNodeCount;
            unsigned int currentArcCount;
            unsigned int maxNodeCount;
            unsigned int maxArcCount;


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

            //TODO BiMap
            unordered_map<ExtNodeID,IntNodeID> internalDistinctNodeIDs;
            unordered_map<IntNodeID,ExtNodeID> swappedInternalDistinctNodeIDs;
            AddedPoints addedStartPoints;
            AddedPoints addedEndPoints;
            unordered_set<string> eliminatedArcs;
            list<ExtFTNode> arcLoops;

            //Network changes
            Arcs internalArcData;
            NewArcs newArcs;
    };
}
#endif // NETWORK_H
