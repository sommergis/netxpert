#include "network.h"

using namespace std;
using namespace NetXpert;

// ColumnMap can override entries in Config
Network::Network(InputArcs _arcsTbl, InputNodes _nodesTbl, ColumnMap _map, Config& cnfg)
{
    //ctor
    try
    {
        fromColName = _map.at("fromColName");
        toColName = _map.at("toColName");
        arcIDColName = _map.at("arcIDColName");
        costColName = _map.at("costColName");
        if (_map.count("capColName") == 1)
            capColName = _map.at("capColName");
        else
            capColName = "";
        if (_map.count("onewayColName") == 1)
            onewayColName = _map.at("onewayColName");
        else
            onewayColName = "";

        nodeIDColName = _map.at("nodeIDColName");
        supplyColName = _map.at("supplyColName");

        netXpertConfig = cnfg;

        internalArcData.reserve(_arcsTbl.size());

        this->arcsTbl = _arcsTbl;
        this->nodesTbl = _nodesTbl;
    }
    catch (exception& ex)
    {
        LOGGER::LogFatal("Network ctor: Error!");
        LOGGER::LogFatal(ex.what());
    }
}

Network::Network(InputArcs _arcsTbl, ColumnMap _map, Config& cnfg)
{
    //ctor
    try
    {
        fromColName = _map.at("fromColName");
        toColName = _map.at("toColName");
        arcIDColName = _map.at("arcIDColName");
        costColName = _map.at("costColName");
        if (_map.count("capColName") == 1)
            capColName = _map.at("capColName");
        else
            capColName = "";
        if (_map.count("onewayColName") == 1)
            onewayColName = _map.at("onewayColName");
        else
            onewayColName = "";

        nodeIDColName = _map.at("nodeIDColName");
        supplyColName = _map.at("supplyColName");

        netXpertConfig = cnfg;

        internalArcData.reserve(_arcsTbl.size());

        this->arcsTbl = _arcsTbl;
    }
    catch (exception& ex)
    {
        LOGGER::LogFatal("Network ctor: Error!");
        LOGGER::LogFatal(ex.what());
    }
}

Network::Network(Arcs arcData, unordered_map<ExtArcID,NodeID> distinctNodeIDs,
                        NodeSupplies _nodeSupplies){}

Network::~Network()
{
    //dtor
}

//public
void Network::AddStartNode(){}
void Network::AddEndNode(){}
void Network::BuildTotalRouteGeometry(){}

Network* Network::ConvertInputNetwork(bool autoClean)
{
    renameNodes();

    processBarriers();

    //Oneway wird nur bei gerichtetem Netzwerk berücksichtigt
    if (!onewayColName.empty() && netXpertConfig.IsDirected == true)
    {
        readNetworkFromTable(autoClean, true);
    }
    else {
        readNetworkFromTable(autoClean, false);
    }
    LOGGER::LogDebug("Count of internalArcData: "+ to_string(internalArcData.size()));
    LOGGER::LogDebug("Count of arcsTbl: "+ to_string(arcsTbl.size()));
    LOGGER::LogDebug("Count of internalDistinctNodes: "+ to_string(internalDistinctNodeIDs.size()));
    LOGGER::LogDebug("Count of nodeSupplies: " + to_string(nodeSupplies.size()));
    LOGGER::LogDebug("Count of nodesTbl: " + to_string(nodesTbl.size()));
    //LOGGER::LogDebug("Last element of eliminatedArcs: " + eliminatedArcs[i]);
    LOGGER::LogDebug("Count of eliminatedArcs: " + to_string(eliminatedArcs.size()));
    //LOGGER::LogDebug("Size of eliminatedArcsCount: "+ to_string(eliminatedArcsCount) );
    return this;
}

void Network::GetOriginalArcData(list<ArcData>& origArcData, list<FTNode>& startEndNodes, bool isDirected){}
void Network::GetOriginalArcDataAndFlow(list<ArcDataAndFlow>& origArcDataAndFlow, list<FTNode>& startEndNodes, bool isDirected){}
string Network::GetOriginalNodeID(unsigned int internalNodeID){}
string Network::GetOriginalStartOrEndNodeID(unsigned int internalNodeID){}
void Network::GetStartOrEndNodeGeometry(Coordinate& coord, unsigned int internalNodeID){}

//private
void Network::renameNodes()
{
    //dynamic dataType = this.arcTbl.Columns[fromColName].DataType;
    set<unsigned int> sortedDistinctNodesSet;
    for (InputArcs::iterator it = arcsTbl.begin(); it != arcsTbl.end(); ++it)
    {
        auto itD = *it;
        //if element is not in set - add it!
        if (sortedDistinctNodesSet.find(itD.fromNode) == sortedDistinctNodesSet.end() )
        {
            sortedDistinctNodesSet.insert(itD.fromNode);
        }
        if (sortedDistinctNodesSet.find(itD.toNode) == sortedDistinctNodesSet.end() )
        {
            sortedDistinctNodesSet.insert(itD.toNode);
        }
    }
    //copy set to vector for index access
    vector<unsigned int> sortedDistinctNodes(sortedDistinctNodesSet.begin(), sortedDistinctNodesSet.end());
    // Count from 0 to sortedDistinctNodes.Count:
    // --> internal NodeIDs start from 1 t;o n.
    for (unsigned int i = 0; i < sortedDistinctNodes.size(); i++)
    {
        // Add [oldNodeID],[internalNodeID]
        internalDistinctNodeIDs.insert( make_pair(to_string(sortedDistinctNodes[i]), i+1) );
        // Add [internalNodeID],[oldNodeID]
        swappedInternalDistinctNodeIDs.insert( make_pair(i+1, to_string(sortedDistinctNodes[i])) );
    }
    // We have to care for the nodes and their supply values also if they are present
    if (nodesTbl.size() > 0)
    {
        for (InputNodes::iterator it = nodesTbl.begin(); it != nodesTbl.end(); it++)
        {
            auto itD = *it;
            string oldNodeID;
            unsigned int internalNodeID;
            double nodeSupply;
            oldNodeID = itD.nodeID;
            nodeSupply = itD.nodeSupply;
            //Get internal node ID from dictionary:
            //add values to NodeSupply
            //throws an exception if not found!
            try {
                internalNodeID = internalDistinctNodeIDs.at(oldNodeID);
            }
            catch (exception& ex)
            {
                LOGGER::LogError("Original node ID " +oldNodeID +" from nodes table not found in arcs!");
            }
            //filter out transshipment nodes -> they're not important here.
            //They are generated when the C++ Solver is called
            try
            {
                if (nodeSupply != 0)
                {
                    NodeSupply sVal {oldNodeID, nodeSupply};
                    nodeSupplies.insert( make_pair(internalNodeID, sVal) );
                }
            }
            catch (exception& ex)
            {
                LOGGER::LogWarning("renameNodes(): Error inserting supply values!");
                LOGGER::LogWarning(ex.what());
            }
        }
    }
    maxNodeCount = static_cast<unsigned int>(internalDistinctNodeIDs.size());
    maxArcCount = static_cast<unsigned int>(arcsTbl.size());
    currentArcCount = maxArcCount;
    currentNodeCount = maxNodeCount;
}

void Network::readNetworkFromTable(bool autoClean, bool oneWay)
{
    unordered_map<FTNode,DuplicateArcData> duplicates;

    for (InputArcs::iterator it = arcsTbl.begin(); it != arcsTbl.end(); ++it)
    {
        InputArc arc = *it;
        // String -> so content of nodeID fields could be string, int or double
        string externalArcID = arc.extArcID;
        string externalStartNode = to_string(arc.fromNode);
        string externalEndNode = to_string(arc.toNode);
        unsigned int internalStartNode;
        unsigned int internalEndNode;
        try{
            internalStartNode = internalDistinctNodeIDs.at(externalStartNode);
            internalEndNode = internalDistinctNodeIDs.at(externalEndNode);
        }
        catch (exception& ex)
        {
            LOGGER::LogError("readNetworkFromTable(): Error getting internal Nodes from internalDistinctNodIDs!");
            LOGGER::LogError(ex.what());
        }

        bool isArcOneway = false;
        if (!onewayColName.empty())
        {
            if (arc.oneway == "Y")
                isArcOneway = true;
        }

        //CHECKS!
        //if found in eliminatedArcs go to next one
        if (eliminatedArcs.count(externalArcID) == 1) //could be eliminated from barriers
        //if (std::find(eliminatedArcs.begin(), eliminatedArcs.end(), externalArcID)!=eliminatedArcs.end())
            continue;
        if (internalStartNode == internalEndNode)
        {
            if (autoClean)
            {
                LOGGER::LogInfo("Loop at "+externalStartNode+ " - " + externalEndNode+ " ignored.");

                ExtFTNode ftNode {externalStartNode, externalEndNode};
                arcLoops.push_back(ftNode);
                eliminatedArcs.insert(externalArcID);
                //eliminatedArcsCount = eliminatedArcsCount + 1;
                continue; // ignorier einfach diese fehlerhafte Kante
            }
            else{
                LOGGER::LogWarning("Loop at "+externalStartNode+ " - " + externalEndNode+ "!");
            }
        }
        if (oneWay)//Global oneway
        {
            if (isArcOneway == false) //arc level oneway Y or N
            {
                //double the from-to-pair
                processArc(arc, internalStartNode, internalEndNode);
                //TODO: check for lookups!
                //processArc(row, externalArcID + "_toFrom", internalEndNode, internalStartNode);
                processArc(arc, internalEndNode, internalStartNode);
            }
            else //regular addition of directed from-to-pair
            {
                processArc(arc, internalStartNode, internalEndNode);
            }
        }
        else
        {
            processArc(arc, internalStartNode, internalEndNode);
        }
    }
    /*
    //TEST
    cout << "Internal Arcs:" << endl;
    for (Arcs::iterator it = internalArcData.begin(); it != internalArcData.end(); ++it)
    {
        auto itD = *it;
        FTNode ft = itD.first;
        ArcData arc = itD.second;
        cout << arc.extArcID << " " << ft.fromNode << " " << ft.toNode << " " << arc.cost << " " << arc.capacity << endl;
    }
    cout << "Eliminated Arcs" << endl;
    for (list<string>::iterator it = eliminatedArcs.begin(); it != eliminatedArcs.end(); ++it)
    {
        string extArcID = *it;
        cout << extArcID << endl;
    }
    cout << "Internal distinct nodes" << endl;
    for (unordered_map<ExtNodeID,NodeID>::iterator it = internalDistinctNodeIDs.begin(); it != internalDistinctNodeIDs.end(); ++it)
    {
        auto itD = *it;
        ExtNodeID extNodeID = itD.first;
        NodeID intNodeID = itD.second;
        cout << extNodeID << " " << intNodeID << endl;
    }*/

    //TODO
    //DBHELPER.EliminatedArcs = this.eliminatedArcs;
}
//TODO: isDirected evtl. von Klasse nehmen, nicht vom Config
void Network::processArc(InputArc arc, unsigned int internalStartNode,
                        unsigned int internalEndNode)
{
    bool isDirected = netXpertConfig.IsDirected;
    string externalArcID = arc.extArcID;

    FTNode inFromToPair {internalStartNode, internalEndNode};

    // Calculates the internal representation of the graph
    // [startNode,endNode],[oldArcID],[cost]
    double cost = arc.cost;
    double cap;
    if (arc.capacity != DOUBLE_NULL)
        cap = arc.capacity;
    else
        cap = DOUBLE_INFINITY; //We do not have a column for caps, so there is no limit

    ArcData arcIDAndCost {externalArcID, cost, cap};

    if (isDirected)
    {
        if (internalArcData.count(inFromToPair) == 0) // 0: key not found, 1: key found
            internalArcData.insert( make_pair(inFromToPair, arcIDAndCost));
        // we have a duplicate arc
        else
        {
            // compare the costs and add only the cheapest
            ArcData outArcData;
            outArcData = internalArcData.at(inFromToPair);
            //LOGGER::LogDebug(to_string(outArcData.cost));
            //LOGGER::LogDebug(to_string(cost));
            if (outArcData.cost > cost)
            {
                //remove old arc
                internalArcData.erase(inFromToPair);
                eliminatedArcs.insert(outArcData.extArcID);
                //eliminatedArcsCount = eliminatedArcsCount + 1;
                //and add new one
                internalArcData.insert( make_pair(inFromToPair, arcIDAndCost));
            }
            //else keep the old one
        }
    }
    else //undirected
    {
        // Hier muss auch das vertauschte FromTo Paar geprüft werden;
        FTNode inToFromPair {inFromToPair.toNode, inFromToPair.fromNode};

        // erst wenn auch bei der Vertauschung auch keine Kante vorhanden ist, füge sie hinzu
        if (internalArcData.count(inFromToPair) +
            internalArcData.count(inToFromPair) == 0)
        {
            internalArcData.insert(make_pair(inFromToPair, arcIDAndCost)); //OK
        }
        // we have a duplicate arc
        else
        {
            // compare the costs and add only the cheapest
            ArcData outArcData;
            // if not found regular, check for swapped
            bool checkSwapped = false;
            if (internalArcData.count(inFromToPair) == 0)
            {
                checkSwapped = true;
                outArcData = internalArcData.at(inToFromPair);
            }
            else
            {
                outArcData = internalArcData.at(inFromToPair);
            }

            if (outArcData.cost > cost)
            {
                if (checkSwapped)
                {
                    //remove old arc
                    internalArcData.erase(inToFromPair);
                    eliminatedArcs.insert(outArcData.extArcID);
                    //eliminatedArcsCount = eliminatedArcsCount + 1;
                    //and add new one
                    internalArcData.insert(make_pair(inToFromPair, arcIDAndCost));
                }
                else
                {
                    //remove old arc
                    internalArcData.erase(inFromToPair);
                    eliminatedArcs.insert(outArcData.extArcID);
                    //eliminatedArcsCount = eliminatedArcsCount + 1;
                    //and add new one
                    internalArcData.insert(make_pair(inFromToPair, arcIDAndCost));
                }
            }
            //else keep the old one
        }
    }
}
void Network::processBarriers(){}
