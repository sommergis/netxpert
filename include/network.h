#ifndef NETWORK_H
#define NETWORK_H

#include "data.h"
#include "logger.h"
#include <string>
#include <algorithm>

using namespace std;
using namespace geos::geom;

namespace NetXpert
{
    class Network
    {
        public:
            Network(Arcs arcData, unordered_map<ExtArcID,NodeID> distinctNodeIDs,
                        NodeSupplies _nodeSupplies);
            Network(InputArcs arcsTbl, ColumnMap _map, Config& cnfg);
            Network(InputArcs arcsTbl, InputNodes nodesTbl, ColumnMap _map, Config& cnfg);
            void AddStartNode();
            void AddEndNode();
            void BuildTotalRouteGeometry();
            Network* ConvertInputNetwork(bool autoClean);
            void GetOriginalArcData(list<ArcData>& origArcData,
                                    list<FTNode>& startEndNodes,
                                    bool isDirected);
            void GetOriginalArcDataAndFlow(list<ArcDataAndFlow>& origArcDataAndFlow,
                                            list<FTNode>& startEndNodes,
                                            bool isDirected);
            string GetOriginalNodeID(uint internalNodeID);
            string GetOriginalStartOrEndNodeID(uint internalNodeID);
            void GetStartOrEndNodeGeometry(Coordinate& coord, uint internalNodeID);

            virtual ~Network();

        private:
            void renameNodes();
            void readNetworkFromTable(bool autoClean, bool oneWay);
            void processArc(InputArc arc, uint internalStartNode,
                                    uint internalEndNode);
            void processBarriers();

            Config netXpertConfig;
            uint currentNodeCount;
            uint currentArcCount;
            uint maxNodeCount;
            uint maxArcCount;

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
            unordered_map<ExtNodeID,NodeID> internalDistinctNodeIDs;
            unordered_map<NodeID,ExtNodeID> swappedInternalDistinctNodeIDs;
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
