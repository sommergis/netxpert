#ifndef NETWORK_H
#define NETWORK_H

#include "dbhelper.h"
#include <algorithm>
#include <geos/geom/CoordinateSequenceFactory.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/opDistance.h>
#include <geos/index/strtree/STRtree.h>
#include <geos/linearref/LengthIndexedLine.h>

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
            Network(InputArcs arcsTbl, ColumnMap _map, Config& cnfg);
            Network(InputArcs arcsTbl, InputNodes nodesTbl, ColumnMap _map, Config& cnfg);
            void AddStartNode(NewNode newNode, int treshold, SQLite::Statement& qry);
            void AddEndNode();
            void BuildTotalRouteGeometry();
            void ConvertInputNetwork(bool autoClean);
            void GetOriginalArcData(list<ArcData>& origArcData,
                                    list<FTNode>& startEndNodes,
                                    bool isDirected);
            void GetOriginalArcDataAndFlow(list<ArcDataAndFlow>& origArcDataAndFlow,
                                            list<FTNode>& startEndNodes,
                                            bool isDirected);
            string GetOriginalNodeID(unsigned int internalNodeID);
            string GetOriginalStartOrEndNodeID(unsigned int internalNodeID);
            void GetStartOrEndNodeGeometry(Coordinate& coord, unsigned int internalNodeID);
            NewSplittedArc GetSplittedClosestNewArcToPoint(Coordinate coord, int treshold,
                                                            bool isPointOnLine, NewArcs& nArcs);
            virtual ~Network();

        private:
            void renameNodes();
            void readNetworkFromTable(bool autoClean, bool oneWay);
            void processArc(InputArc arc, unsigned int internalStartNode,
                                    unsigned int internalEndNode);
            void processBarriers();

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

            Arcs internalArcData;
            unordered_map<ExtNodeID,IntNodeID> internalDistinctNodeIDs;
            unordered_map<IntNodeID,ExtNodeID> swappedInternalDistinctNodeIDs;
            AddedPoints addedStartPoints;
            AddedPoints addedEndPoints;
            unordered_set<string> eliminatedArcs;
            list<ExtFTNode> arcLoops;

            //Network changes
            Arcs oldArcs;
            NewArcs newArcs;
    };
}
#endif // NETWORK_H
