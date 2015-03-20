#include "network.h"

using namespace std;
using namespace NetXpert;

// ColumnMap can override entries in Config
Network::Network(InputArcs _arcsTbl, InputNodes _nodesTbl, ColumnMap _map, Config& cnfg)
{
    //ctor
    try
    {
        arcIDColName = _map.arcIDColName;
        fromColName = _map.fromColName;
        toColName = _map.toColName;
        costColName = _map.costColName;
        if (!_map.capColName.empty())
            capColName = _map.capColName;
        else
            capColName = "";
        if (!_map.onewayColName.empty())
            onewayColName = _map.onewayColName;
        else
            onewayColName = "";

        nodeIDColName = _map.nodeIDColName;
        supplyColName = _map.supplyColName;

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
        arcIDColName = _map.arcIDColName;
        fromColName = _map.fromColName;
        toColName = _map.toColName;
        costColName = _map.costColName;
        if (!_map.capColName.empty())
            capColName = _map.capColName;
        else
            capColName = "";
        if (!_map.onewayColName.empty())
            onewayColName = _map.onewayColName;
        else
            onewayColName = "";

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

Network::Network(Arcs arcData, unordered_map<ExtArcID,IntNodeID> distinctNodeIDs,
                        NodeSupplies _nodeSupplies){}

Network::~Network()
{
    //dtor
}

//public
void Network::AddStartNode(NewNode newNode, int treshold, SQLite::Statement& qry)
{
    unordered_map<string, FTNode> newSegments;
    newSegments.reserve(2);

    const string extNodeID = newNode.extNodeID;
    const Coordinate startPoint = newNode.coord;
    const double nodeSupply = newNode.supply;

    //if (!startPoint.x)
    //    throw new InvalidValueException("Start coordinate must not be null!");

    //1. Suche die nächste Line zum temporaeren Punkt
    ClosestArcAndPoint closestArcAndPoint = DBHELPER::GetClosestArcFromPoint(startPoint,
            treshold, qry);
    const string extArcID = closestArcAndPoint.extArcID;
    const Coordinate closestPoint = closestArcAndPoint.closestPoint;

}
void Network::AddEndNode(){}
void Network::BuildTotalRouteGeometry(){}

void Network::ConvertInputNetwork(bool autoClean)
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
}

void Network::GetOriginalArcData(list<ArcData>& origArcData, list<FTNode>& startEndNodes, bool isDirected){}
void Network::GetOriginalArcDataAndFlow(list<ArcDataAndFlow>& origArcDataAndFlow, list<FTNode>& startEndNodes, bool isDirected){}

string Network::GetOriginalNodeID(unsigned int internalNodeID)
{
    string externalNodeID = "";
    try
    {
        externalNodeID = swappedInternalDistinctNodeIDs.at(internalNodeID);
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Original node ID " +to_string(internalNodeID) +" could not be looked up!");
        throw ex;
    }
    return externalNodeID;
}
string Network::GetOriginalStartOrEndNodeID(unsigned int internalNodeID)
{
    AddedPoint externalNode;
    try
    {
        if (addedStartPoints.count(internalNodeID) == 1)
            externalNode = addedStartPoints.at(internalNodeID);
        else
            externalNode = addedEndPoints.at(internalNodeID);
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Original start/end node ID " +to_string(internalNodeID) +" could not be looked up!");
        throw ex;
    }
    return externalNode.extNodeID;
}
void Network::GetStartOrEndNodeGeometry(Coordinate& coord, unsigned int internalNodeID)
{
    AddedPoint externalNode;
    try
    {
        if (addedStartPoints.count(internalNodeID) == 1)
            externalNode = addedStartPoints.at(internalNodeID);
        else
            externalNode = addedEndPoints.at(internalNodeID);
    }
    catch (exception& ex)
    {
        LOGGER::LogError("Original start/end node ID " +to_string(internalNodeID) +" could not be looked up!");
        throw ex;
    }
    coord = externalNode.coord;
}

NewSplittedArc Network::GetSplittedClosestNewArcToPoint(Coordinate coord, int treshold,
                                                            bool isPointOnLine, NewArcs& nArcs)
{
    try
    {
        const double tolerance = 0.001;
        GeometryFactory gFac;
        shared_ptr<const Point> p ( gFac.createPoint(coord) );
        shared_ptr<const Geometry> gPtr ( dynamic_pointer_cast<const Geometry>(p) );
        shared_ptr<const Geometry> buf ( gPtr->buffer(treshold) );
        //must not defined as smart_pointer!
        const Envelope* bufEnv = buf->getEnvelopeInternal();

        map<double, pair<FTNode, NewArc> > distanceTbl;

        /*// search in spatial index for relevant geometry
        // with buffer around point (treshold)
        auto bufGeom = gPtr->buffer(treshold);
        const auto envBufGeom->getEnvelopeInternal();
        newArcsSindex.insert(envBufGeom, gPtr);*/

        //1. Generate Distance Table
        for ( auto nArc : nArcs )
        {
            FTNode key = nArc.first;
            NewArc val = nArc.second;
            Geometry* g2Ptr = val.arcGeom.get();

            // "spatial index like":
            // calculate distance table only for those geometries
            // that envelopes do intersect

            //must not defined as smart_pointer!
            const Envelope* g2Env = g2Ptr->getEnvelopeInternal() ;

            if(bufEnv->intersects(g2Env)) //get for raw pointer
            {
                DistanceOp distCalc(*gPtr, *g2Ptr);
                //cout << distCalc.distance() << endl << val.arcGeom.toString() << endl;
                distanceTbl.insert( make_pair(distCalc.distance(),
                                    make_pair(key, val) ) );
            }
        }

        //take the nearest line = first of distanceTable, because it's ordered by distance
        /*for (auto item : distanceTbl)
        {
            //auto t = *item.second;
            cout << item.first << endl << item.second.arcGeom->toString() << endl;
        }*/

        auto elem = *distanceTbl.begin();
        double dist = elem.first;
        FTNode nearestArcKey = elem.second.first;
        NewArc nearestArc = elem.second.second;
        //must not defined as smart_pointer!
        shared_ptr<const Geometry> nGeomPtr ( nearestArc.arcGeom );

        if (!isPointOnLine) //default
        {
            //make a split blade (=line)
            //do the split
            LengthIndexedLine idxLine = geos::linearref::LengthIndexedLine( nGeomPtr.get() );//get for raw pointer

            double pointIdx = idxLine.indexOf(coord);
            double startIdx = idxLine.getStartIndex();
            double endIdx = idxLine.getEndIndex();

            //cout << "Indexes: "<< endl;
            //cout << pointIdx << endl << startIdx << endl << endIdx << endl;

            shared_ptr<Geometry> seg1 ( idxLine.extractLine(startIdx, pointIdx) );
            shared_ptr<Geometry> seg2 ( idxLine.extractLine(pointIdx, endIdx) );

            shared_ptr<MultiLineString> mLine ( gFac.createMultiLineString(
                                                  vector<Geometry*>{ seg1.get() , seg2.get() } )
                                                );

            //cout << seg1->toString() << endl << seg2->toString() << endl;

            NewSplittedArc result;
            result.fromNode = nearestArcKey.fromNode;
            result.toNode = nearestArcKey.toNode;
            result.cost = nearestArc.cost;
            result.capacity = nearestArc.capacity;
            result.arcGeom = mLine;

            return result;
        }
        else
        {
            //make a split blade (=line) with tolerance
            //do the split
        }
        //DEBUG
        /*for (auto item : distanceTbl)
        {
            cout << item.first << endl << item.second.arcGeom.toString() << endl;
        }*/
    }
    catch (std::exception& ex)
    {
        LOGGER::LogError( "Error splitting new line!" );
        LOGGER::LogError( ex.what() );
        throw ex;
    }
}


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
            string extNodeID;
            unsigned int internalNodeID;
            double nodeSupply;
            extNodeID = itD.extNodeID;
            nodeSupply = itD.nodeSupply;
            //Get internal node ID from dictionary:
            //add values to NodeSupply
            //throws an exception if not found!
            try {
                internalNodeID = internalDistinctNodeIDs.at(extNodeID);
            }
            catch (exception& ex)
            {
                LOGGER::LogError("Original node ID " +extNodeID +" from nodes table not found in arcs!");
                throw ex;
            }
            //filter out transshipment nodes -> they're not important here.
            //They are generated when the C++ Solver is called
            try
            {
                if (nodeSupply != 0)
                {
                    NodeSupply sVal {extNodeID, nodeSupply};
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
            throw ex;
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
